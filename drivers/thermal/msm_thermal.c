/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/msm_tsens.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/msm_tsens.h>
#include <linux/msm_thermal.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <mach/cpufreq.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/err.h>

#define MAX_CURRENT_UA 1000000
#define MAX_RAILS 5

static bool msm_thermal_probed;
static int ocr_rail_cnt;
unsigned int temp_threshold = 70;
module_param(temp_threshold, int, 0755);
static bool ocr_enabled;
static bool ocr_nodes_called;
static bool ocr_probed;
static DEFINE_MUTEX(ocr_mutex);
static int rails_cnt;
static int max_tsens_num;
static int *tsens_id_map;

unsigned int temp_scan_interval = 500;
module_param(temp_scan_interval, int, 0644);

static struct thermal_info {
	uint32_t cpuinfo_max_freq;
	uint32_t limited_max_freq;
	unsigned int safe_diff;
	bool throttling;
	bool pending_change;
} info = {
	.cpuinfo_max_freq = LONG_MAX,
	.limited_max_freq = LONG_MAX,
	.safe_diff = 5,
	.throttling = false,
	.pending_change = false,
};

struct rail {
        const char *name;
        int32_t curr_level;
	struct regulator *reg;
};

struct psm_rail {
        const char *name;  
        uint8_t init;   
	uint8_t mode;
        struct kobj_attribute mode_attr;
	struct regulator *phase_reg;
       	struct rpm_regulator *reg;
        struct attribute_group attr_gp;
};

static struct rail *rails;
static struct psm_rail *ocr_rails;

enum ocr_request {
	OPTIMUM_CURRENT_MIN,
	OPTIMUM_CURRENT_MAX,
	OPTIMUM_CURRENT_NR,
};

enum thermal_freqs {
	FREQ_HELL		= 729600,
	FREQ_VERY_HOT		= 1036800,
	FREQ_HOT		= 1497600,
	FREQ_WARM		= 1958400,
};

enum threshold_levels {
	LEVEL_HELL		= 12,
	LEVEL_VERY_HOT		= 9,
	LEVEL_HOT		= 5,
};

#define OCR_RW_ATTRIB(_rail, ko_attr, j, _name) \
	ko_attr.attr.name = __stringify(_name); \
	ko_attr.attr.mode = 644; \
	ko_attr.show = ocr_reg_##_name##_show; \
	ko_attr.store = ocr_reg_##_name##_store; \
	sysfs_attr_init(&ko_attr.attr); \
	_rail.attr_gp.attrs[j] = &ko_attr.attr;

#define PSM_REG_MODE_FROM_ATTRIBS(attr) \
	(container_of(attr, struct psm_rail, mode_attr));

static struct msm_thermal_data msm_thermal_info;

static struct delayed_work check_temp_work;

unsigned short get_threshold(void)
{
	return temp_threshold;
}

static int request_optimum_current(struct psm_rail *rail, enum ocr_request req)
{
	int ret = 0;

	if ((!rail) || (req >= OPTIMUM_CURRENT_NR) ||
		(req < 0)) {
		pr_err("%s:%s Invalid input\n", KBUILD_MODNAME, __func__);
		ret = -EINVAL;
		goto request_ocr_exit;
	}

	ret = regulator_set_optimum_mode(rail->phase_reg,
		(req == OPTIMUM_CURRENT_MAX) ? MAX_CURRENT_UA : 0);
	if (ret < 0) {
		pr_err("%s: Optimum current request failed\n", KBUILD_MODNAME);
		goto request_ocr_exit;
	}
	ret = 0; /*regulator_set_optimum_mode returns the mode on success*/
	pr_debug("%s: Requested optimum current mode: %d\n",
		KBUILD_MODNAME, req);

request_ocr_exit:
	return ret;
}

static int ocr_set_mode_all(enum ocr_request req)
{       
        int i = 0;
	int ret = 0;

	for (i = 0; i < ocr_rail_cnt; i++) {
		if (ocr_rails[i].mode == req)
			continue;
		ret = request_optimum_current(&ocr_rails[i], req);
		if (ret)
			goto ocr_set_mode_exit;
		ocr_rails[i].mode = req;
	}

ocr_set_mode_exit:
	return ret;
}

static int ocr_reg_mode_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct psm_rail *reg = PSM_REG_MODE_FROM_ATTRIBS(attr);
	return snprintf(buf, PAGE_SIZE, "%d\n", reg->mode);
}

static ssize_t ocr_reg_mode_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	int val = 0;
	struct psm_rail *reg = PSM_REG_MODE_FROM_ATTRIBS(attr);

	if (!ocr_enabled)
		return count;

	mutex_lock(&ocr_mutex);
	ret = kstrtoint(buf, 10, &val);
	if (ret) {
		pr_err("%s: Invalid input %s for mode\n",
			KBUILD_MODNAME, buf);
		goto done_ocr_store;
	}

	if ((val != OPTIMUM_CURRENT_MAX) &&
		(val != OPTIMUM_CURRENT_MIN)) {
		pr_err("%s: Invalid value %d for mode\n",
			KBUILD_MODNAME, val);
		goto done_ocr_store;
	}

	if (val != reg->mode) {
		ret = request_optimum_current(reg, val);
		if (ret)
			goto done_ocr_store;
		reg->mode = val;
	}

done_ocr_store:
	mutex_unlock(&ocr_mutex);
	return count;
}

static int msm_thermal_cpufreq_callback(struct notifier_block *nfb,
		unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;

	if (event != CPUFREQ_ADJUST && !info.pending_change)
		return 0;

	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
		info.limited_max_freq);

	return 0;
}

static struct notifier_block msm_thermal_cpufreq_notifier = {
	.notifier_call = msm_thermal_cpufreq_callback,
};

static void limit_cpu_freqs(uint32_t max_freq)
{
        int i = 0;
	unsigned int cpu;

	if (info.limited_max_freq == max_freq)
		return;

	info.limited_max_freq = max_freq;

	info.pending_change = true;

		rails[i].curr_level = -1;
		rails[i].reg = NULL;
		i++;
        get_online_cpus();
	for_each_online_cpu(cpu)
	{
		cpufreq_update_policy(cpu);
		pr_info("%s: Setting cpu%d max frequency to %d\n",
				KBUILD_MODNAME, cpu, info.limited_max_freq);
	}
        put_online_cpus();

	info.pending_change = false;
}

static int do_ocr(void)
{
	struct tsens_device tsens_dev;
	long temp = 0;
	int ret = 0;
	int i = 0, j = 0;
	int auto_cnt = 0;

	if (!ocr_enabled)
		return ret;

	mutex_lock(&ocr_mutex);
	for (i = 0; i < max_tsens_num; i++) {
		tsens_dev.sensor_num = tsens_id_map[i];
		ret = tsens_get_temp(&tsens_dev, &temp);
		if (ret) {
			pr_debug("%s: Unable to read TSENS sensor %d\n",
					__func__, tsens_dev.sensor_num);
			auto_cnt++;
			continue;
		}

		if (temp > msm_thermal_info.ocr_temp_degC) {
			if (ocr_rails[0].init != OPTIMUM_CURRENT_NR)
				for (j = 0; j < ocr_rail_cnt; j++)
					ocr_rails[j].init = OPTIMUM_CURRENT_NR;
			ret = ocr_set_mode_all(OPTIMUM_CURRENT_MAX);
			if (ret)
				pr_err("Error setting max optimum current\n");
			goto do_ocr_exit;
		} else if (temp <= (msm_thermal_info.ocr_temp_degC -
			msm_thermal_info.ocr_temp_hyst_degC))
			auto_cnt++;
	}

	if (auto_cnt == max_tsens_num ||
		ocr_rails[0].init != OPTIMUM_CURRENT_NR) {
		/* 'init' not equal to OPTIMUM_CURRENT_NR means this is the
		** first polling iteration after device probe. During first
		** iteration, if temperature is less than the set point, clear
		** the max current request made and reset the 'init'.
		*/
		if (ocr_rails[0].init != OPTIMUM_CURRENT_NR)
			for (j = 0; j < ocr_rail_cnt; j++)
				ocr_rails[j].init = OPTIMUM_CURRENT_NR;
		ret = ocr_set_mode_all(OPTIMUM_CURRENT_MIN);
		if (ret) {
			pr_err("Error setting min optimum current\n");
			goto do_ocr_exit;
		}
	}

do_ocr_exit:
	mutex_unlock(&ocr_mutex);
	return ret;
}

static int msm_thermal_add_ocr_nodes(void)
{
	struct kobject *module_kobj = NULL;
	struct kobject *ocr_kobj = NULL;
	struct kobject *ocr_reg_kobj[MAX_RAILS] = {0};
	int rc = 0;
	int i = 0;

	if (!ocr_probed) {
		ocr_nodes_called = true;
		return rc;
	}

	if (ocr_probed && ocr_rail_cnt == 0)
		return rc;

	module_kobj = kset_find_obj(module_kset, KBUILD_MODNAME);
	if (!module_kobj) {
		pr_err("%s: cannot find kobject for module %s\n",
			__func__, KBUILD_MODNAME);
		rc = -ENOENT;
		goto ocr_node_exit;
	}

	ocr_kobj = kobject_create_and_add("opt_curr_req", module_kobj);
	if (!ocr_kobj) {
		pr_err("%s: cannot create ocr kobject\n", KBUILD_MODNAME);
		rc = -ENOMEM;
		goto ocr_node_exit;
	}

	for (i = 0; i < ocr_rail_cnt; i++) {
		ocr_reg_kobj[i] = kobject_create_and_add(ocr_rails[i].name,
					ocr_kobj);
		if (!ocr_reg_kobj[i]) {
			pr_err("%s: cannot create for kobject for %s\n",
					KBUILD_MODNAME, ocr_rails[i].name);
			rc = -ENOMEM;
			goto ocr_node_exit;
		}
		ocr_rails[i].attr_gp.attrs = kzalloc( \
				sizeof(struct attribute *) * 2, GFP_KERNEL);
		if (!ocr_rails[i].attr_gp.attrs) {
			rc = -ENOMEM;
			goto ocr_node_exit;
		}

		OCR_RW_ATTRIB(ocr_rails[i], ocr_rails[i].mode_attr, 0, mode);
		ocr_rails[i].attr_gp.attrs[1] = NULL;

		rc = sysfs_create_group(ocr_reg_kobj[i], &ocr_rails[i].attr_gp);
		if (rc) {
			pr_err("%s: cannot create attribute group for %s\n",
				KBUILD_MODNAME, ocr_rails[i].name);
			goto ocr_node_exit;
		}
	}

ocr_node_exit:
	if (rc) {
		for (i = 0; i < ocr_rail_cnt; i++) {
			if (ocr_reg_kobj[i])
				kobject_del(ocr_reg_kobj[i]);
			if (ocr_rails[i].attr_gp.attrs) {
				kfree(ocr_rails[i].attr_gp.attrs);
				ocr_rails[i].attr_gp.attrs = NULL;
			}
		}
		if (ocr_kobj)
			kobject_del(ocr_kobj);
	}
	return rc;
}

static int ocr_reg_init(struct platform_device *pdev)
{
	int ret = 0;
	int i, j;

	for (i = 0; i < ocr_rail_cnt; i++) {
		/* Check if vdd_restriction has already initialized any
		 * regualtor handle. If so use the same handle.*/
		for (j = 0; j < rails_cnt; j++) {
			if (!strcmp(ocr_rails[i].name, rails[j].name)) {
				if (rails[j].reg == NULL)
					break;
				ocr_rails[i].phase_reg = rails[j].reg;
				goto reg_init;
			}

		}
		ocr_rails[i].phase_reg = devm_regulator_get(&pdev->dev,
					ocr_rails[i].name);
		if (IS_ERR_OR_NULL(ocr_rails[i].phase_reg)) {
			ret = PTR_ERR(ocr_rails[i].phase_reg);
			if (ret != -EPROBE_DEFER) {
				pr_err("%s, could not get regulator: %s\n",
					__func__, ocr_rails[i].name);
				ocr_rails[i].phase_reg = NULL;
				ocr_rails[i].mode = 0;
				ocr_rails[i].init = 0;
			}
			return ret;
		}
reg_init:
		ocr_rails[i].mode = OPTIMUM_CURRENT_MIN;
	}
	return ret;
}

static int probe_ocr(struct device_node *node, struct msm_thermal_data *data,
		struct platform_device *pdev)
{
	int ret = 0;
	int j = 0;
	char *key = NULL;

	if (ocr_probed) {
		pr_info("%s: Nodes already probed\n",
			__func__);
		goto read_ocr_exit;
	}
	ocr_rails = NULL;

	key = "qti,pmic-opt-curr-temp";
	ret = of_property_read_u32(node, key, &data->ocr_temp_degC);
	if (ret)
		goto read_ocr_fail;

	key = "qti,pmic-opt-curr-temp-hysteresis";
	ret = of_property_read_u32(node, key, &data->ocr_temp_hyst_degC);
	if (ret)
		goto read_ocr_fail;

	key = "qti,pmic-opt-curr-regs";
	ocr_rail_cnt = of_property_count_strings(node, key);
	ocr_rails = kzalloc(sizeof(struct psm_rail) * ocr_rail_cnt,
			GFP_KERNEL);
	if (!ocr_rails) {
		pr_err("%s: Fail to allocate memory for ocr rails\n", __func__);
		ocr_rail_cnt = 0;
		return -ENOMEM;
	}

	for (j = 0; j < ocr_rail_cnt; j++) {
		ret = of_property_read_string_index(node, key, j,
				&ocr_rails[j].name);
		if (ret)
			goto read_ocr_fail;
		ocr_rails[j].phase_reg = NULL;
		ocr_rails[j].init = OPTIMUM_CURRENT_MAX;
	}

	if (ocr_rail_cnt) {
		ret = ocr_reg_init(pdev);
		if (ret) {
			pr_info("%s:Failed to get regulators. KTM continues.\n",
					__func__);
			goto read_ocr_fail;
		}
		ocr_enabled = true;
		ocr_nodes_called = false;
		/*
		 * Vote for max optimum current by default until we have made
		 * our first temp reading
		 */
		if (ocr_set_mode_all(OPTIMUM_CURRENT_MAX))
			pr_err("Set max optimum current failed\n");
	}

read_ocr_fail:
	ocr_probed = true;
	if (ret) {
		dev_info(&pdev->dev,
			"%s:Failed reading node=%s, key=%s. KTM continues\n",
			__func__, node->full_name, key);
		if (ocr_rails)
			kfree(ocr_rails);
		ocr_rails = NULL;
		ocr_rail_cnt = 0;
	}
	if (ret == -EPROBE_DEFER)
		ocr_probed = false;
read_ocr_exit:
	return ret;
}

static void check_temp(struct work_struct *work)
{
	struct tsens_device tsens_dev;
	uint32_t freq = 0;
	long temp = 0;

        if (!msm_thermal_probed)
		return;

	tsens_dev.sensor_num = msm_thermal_info.sensor_id;
	tsens_get_temp(&tsens_dev, &temp);

	if (info.throttling)
	{
		if (temp < (temp_threshold - info.safe_diff))
		{
			limit_cpu_freqs(info.cpuinfo_max_freq);
			info.throttling = false;
			goto reschedule;
		}
	}

	if (temp >= temp_threshold + LEVEL_HELL)
		freq = FREQ_HELL;
	else if (temp >= temp_threshold + LEVEL_VERY_HOT)
		freq = FREQ_VERY_HOT;
	else if (temp >= temp_threshold + LEVEL_HOT)
		freq = FREQ_HOT;
	else if (temp > temp_threshold)
		freq = FREQ_WARM;

	if (freq)
	{
		limit_cpu_freqs(freq);

        	if (!info.throttling)
			info.throttling = true;
	}

        do_ocr();

reschedule:
	schedule_delayed_work_on(0, &check_temp_work, msecs_to_jiffies(temp_scan_interval));
}

static int __devinit msm_thermal_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
        char *key = NULL;
	struct device_node *node = pdev->dev.of_node;
	struct msm_thermal_data data;

	memset(&data, 0, sizeof(struct msm_thermal_data));

	ret = of_property_read_u32(node, "qcom,sensor-id", &data.sensor_id);
	if (ret)
		return ret;

	WARN_ON(data.sensor_id >= TSENS_MAX_SENSORS);

	memcpy(&msm_thermal_info, &data, sizeof(struct msm_thermal_data));

	INIT_DELAYED_WORK(&check_temp_work, check_temp);
        schedule_delayed_work_on(0, &check_temp_work, 5);

	cpufreq_register_notifier(&msm_thermal_cpufreq_notifier,
			CPUFREQ_POLICY_NOTIFIER);

        ret = probe_ocr(node, &data, pdev);
	if (ret == -EPROBE_DEFER)
		goto fail;

        if (ocr_nodes_called) {
		msm_thermal_add_ocr_nodes();
		ocr_nodes_called = false;
	}

	return ret;
fail:
	if (ret)
		pr_err("Failed reading node=%s, key=%s. err:%d\n",
			node->full_name, key, ret);

	return ret;
}

static int msm_thermal_dev_remove(struct platform_device *pdev)
{
	cpufreq_unregister_notifier(&msm_thermal_cpufreq_notifier,
                        CPUFREQ_POLICY_NOTIFIER);
	return 0;
}

static struct of_device_id msm_thermal_match_table[] = {
	{.compatible = "qcom,msm-thermal"},
	{},
};

static struct platform_driver msm_thermal_device_driver = {
	.probe = msm_thermal_dev_probe,
        .remove = msm_thermal_dev_remove,
	.driver = {
		.name = "msm-thermal",
		.owner = THIS_MODULE,
		.of_match_table = msm_thermal_match_table,
	},
};

static int __init msm_thermal_device_init(void)
{
        msm_thermal_add_ocr_nodes();
	return platform_driver_register(&msm_thermal_device_driver);
}

static void __exit msm_thermal_device_exit(void)
{
	platform_driver_unregister(&msm_thermal_device_driver);
}

late_initcall(msm_thermal_device_init);
module_exit(msm_thermal_device_exit);
