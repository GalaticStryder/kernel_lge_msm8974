/*
 * Copyright 2016 - Yuri Sh. <yuri@bynet.co.il>
 * Copyright 2016 - Ícaro Hoff <icarohoff@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * Credits / Changelog:
 *
 * Version 1.0: Initial build by Paul Reioux.
 * Version 1.1: Added 1800ma limit to table by Dorimanx.
 * Version 1.2: Added fake AC interface by Mankindtw@xda and Dorimanx.
 * Version 1.3: Misc fixes to force AC and allowed real 1800mA max.
 * Version 1.4: Added usage of custom mA values for max charging power.
 * Version 1.5: Trying to perfect fast charge auto on/off.
 * Version 1.9: Added auto fast charge on/off based on battery %, if above 95% then fast charge is off,
 * when battery is below 95% and fast charge was on, then it will be enabled again.
 * Version 2.0: Guard with mutex all functions that use values from other code to prevent race and bug.
 * Version 2.1: Corect Mutex guards in code for fastcharge.
 * Version 2.2: Allow to charge on 900mA lock.
 * Version 2.3: Added more checks to thermal mitigation functions and corrected code style.
 * Version 2.4: Allowed full 2000mA to be set in charger driver.
 * Version 2.5: Fixed broken mitigation set if USB is connected.
 * Version 2.6: Adapted force fast charge to LP kernel source.
 * Version 2.7: Fixed activation of force fast charge when no power connected.
 * Version 2.8: Fixed wrong set for 2000mA, fixed missing 900mA for charge prepare function.
 * Version 2.9: Allowed higher ma set on misc chargers.
 * Version 3.0: Guard max charge for OTG driver.
 * Version 3.1: Fix bugs in mitigation functions and lge_charging_scenario.
 * Version 3.2: Reviewed all the code and cleaned/adjusted everything that was possible.
 */

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/fastchg.h>

/*
 * The possible values for "force_fast_charge" are:
 *
 * 0 - Disabled (default)
 * 1 - Substitute AC to USB unconditionally
 * 2 - Custom
 *
 */

int force_fast_charge;
int force_fast_charge_temp;
int fast_charge_level;
int force_fast_charge_on_off;

static ssize_t force_fast_charge_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", force_fast_charge);
}

static ssize_t force_fast_charge_store(struct kobject *kobj,
			struct kobj_attribute *attr, const char *buf,
			size_t count)
{

	int new_force_fast_charge;

	sscanf(buf, "%du", &new_force_fast_charge);

	switch(new_force_fast_charge) {
		case FAST_CHARGE_DISABLED:
		case FAST_CHARGE_FORCE_AC:
		case FAST_CHARGE_FORCE_CUSTOM_MA:
			force_fast_charge = new_force_fast_charge;
			force_fast_charge_temp = new_force_fast_charge;
			force_fast_charge_on_off = new_force_fast_charge;
			return count;
		default:
			return -EINVAL;
	}
}

static ssize_t charge_level_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", fast_charge_level);
}

static ssize_t charge_level_store(struct kobject *kobj,
			struct kobj_attribute *attr, const char *buf,
			size_t count)
{

	int new_charge_level;

	sscanf(buf, "%du", &new_charge_level);

	switch (new_charge_level) {
		case FAST_CHARGE_500:
		case FAST_CHARGE_900:
		case FAST_CHARGE_1200:
		case FAST_CHARGE_1600:
		case FAST_CHARGE_1800:
		case FAST_CHARGE_2000:
			fast_charge_level = new_charge_level;
			return count;
		default:
			return -EINVAL;
	}
	return -EINVAL;
}

static ssize_t available_charge_levels_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", FAST_CHARGE_LEVELS);
}

static ssize_t version_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", FAST_CHARGE_VERSION);
}

static struct kobj_attribute version_attribute =
	__ATTR(version, 0444, version_show, NULL);

static struct kobj_attribute available_charge_levels_attribute =
	__ATTR(available_charge_levels, 0444,
		available_charge_levels_show, NULL);

static struct kobj_attribute fast_charge_level_attribute =
	__ATTR(fast_charge_level, 0666,
		charge_level_show,
		charge_level_store);

static struct kobj_attribute force_fast_charge_attribute =
	__ATTR(force_fast_charge, 0666,
		force_fast_charge_show,
		force_fast_charge_store);

static struct attribute *force_fast_charge_attrs[] = {
	&force_fast_charge_attribute.attr,
	&fast_charge_level_attribute.attr,
	&available_charge_levels_attribute.attr,
	&version_attribute.attr,
	NULL,
};

static struct attribute_group force_fast_charge_attr_group = {
	.attrs = force_fast_charge_attrs,
};

static struct kobject *force_fast_charge_kobj;

int force_fast_charge_init(void)
{
	int force_fast_charge_retval;

	 /* Forced fast charge is disabled by default */
	force_fast_charge = FAST_CHARGE_DISABLED;
	force_fast_charge_temp = FAST_CHARGE_DISABLED;
	force_fast_charge_on_off = FAST_CHARGE_DISABLED;
	fast_charge_level = FAST_CHARGE_1600;

	force_fast_charge_kobj
		= kobject_create_and_add("fast_charge", kernel_kobj);

	if (!force_fast_charge_kobj) {
		return -ENOMEM;
	}

	force_fast_charge_retval
		= sysfs_create_group(force_fast_charge_kobj,
				&force_fast_charge_attr_group);

	if (force_fast_charge_retval)
		kobject_put(force_fast_charge_kobj);

	if (force_fast_charge_retval)
		kobject_put(force_fast_charge_kobj);

	return (force_fast_charge_retval);
}

void force_fast_charge_exit(void)
{
	kobject_put(force_fast_charge_kobj);
}

module_init(force_fast_charge_init);
module_exit(force_fast_charge_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jean-Pierre Rasquin <yank555.lu@gmail.com>");
MODULE_AUTHOR("Paul Reioux <reioux@gmail.com>");
MODULE_AUTHOR("Yuri Sh. <yuri@bynet.co.il>");
MODULE_AUTHOR("Ícaro Hoff <icarohoff@gmail.com>");
MODULE_DESCRIPTION("FFC hack for Android");
