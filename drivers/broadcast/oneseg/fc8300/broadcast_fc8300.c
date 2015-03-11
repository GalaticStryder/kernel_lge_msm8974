#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h> 

#include <linux/err.h>
//#include <mach/msm_xo.h>

#include "broadcast_dmb_typedef.h"
#include "broadcast_dmb_drv_ifdef.h"
#include "broadcast_fc8300.h"
#include "fci_types.h"
#include "fci_oal.h"
#include "fc8300_drv_api.h"

//TcpalSemaphore_t fc8300DrvSem;

//static struct msm_xo_voter *xo_handle_tcc;

/*#define _NOT_USE_WAKE_LOCK_*/
#define USE_PM8941_XO_A2
#if defined(USE_PM8941_XO_A2)
#include <linux/clk.h>
#endif

struct broadcast_fc8300_ctrl_data
{
	int			pwr_state;
	struct wake_lock	wake_lock;
	struct spi_device	*spi_dev;
	struct i2c_client	*pclient;
#if defined(USE_PM8941_XO_A2)
    struct clk* pm8941_xo_a2_192000_clk;
#endif
};

static struct broadcast_fc8300_ctrl_data  IsdbCtrlInfo;
int broadcast_dmb_drv_start(void);

struct i2c_client*	FCI_GET_I2C_DRIVER(void)
{
	return IsdbCtrlInfo.pclient;
}

int fc8300_power_on(void)
{
	if(IsdbCtrlInfo.pwr_state != 1)
	{
//		int rc;
#ifndef _NOT_USE_WAKE_LOCK_
		wake_lock(&IsdbCtrlInfo.wake_lock);
#endif
		tunerbb_drv_hw_init();

/*
		rc = msm_xo_mode_vote(xo_handle_tcc, MSM_XO_MODE_ON);
		if(rc < 0) {
			pr_err("Configuring MSM_XO_MODE_ON failed (%d)\n", rc);
			msm_xo_put(xo_handle_tcc);
			return FALSE;
		}
*/
#ifdef USE_PM8941_XO_A2
        print_log(NULL,"[1seg] LGE_BROADCAST_DMB_IOCTL_ON IS_ERR_OR_NULL(clk) first get clk!!!\n");
        if ( !IS_ERR_OR_NULL(IsdbCtrlInfo.pm8941_xo_a2_192000_clk) )
        {
            int ret = -1;
            ret = clk_prepare_enable(IsdbCtrlInfo.pm8941_xo_a2_192000_clk);
            if (ret) {
                print_log(NULL,"[1seg] LGE_BROADCAST_DMB_IOCTL_ON enable clock error!!!\n");
                return -1;
            }
        }
#endif /* USE_PM8941_XO_A2 */

	}
	else
	{
		print_log(NULL, "aready on!! \n");
	}

	IsdbCtrlInfo.pwr_state = 1;
	return OK;
}

int fc8300_is_power_on()
{
	return (int)IsdbCtrlInfo.pwr_state;
}

int fc8300_power_off(void)
{
	if(IsdbCtrlInfo.pwr_state == 0)
	{
		print_log(NULL, "Isdb_tcc3530_power is immediately off\n");
		return OK;
	}
	else
	{
/*
		if(xo_handle_tcc != NULL) {
		    msm_xo_mode_vote(xo_handle_tcc, MSM_XO_MODE_OFF);
		}
*/		
		print_log(NULL, "Isdb_tcc3530_power_off\n");
		tunerbb_drv_hw_deinit();
	}

#ifndef _NOT_USE_WAKE_LOCK_
	wake_unlock(&IsdbCtrlInfo.wake_lock);
#endif

#ifdef USE_PM8941_XO_A2
    if ( !IS_ERR_OR_NULL(IsdbCtrlInfo.pm8941_xo_a2_192000_clk) )
    {
        clk_disable_unprepare(IsdbCtrlInfo.pm8941_xo_a2_192000_clk);
    }
#endif /* USE_PM8941_XO_A2 */

	IsdbCtrlInfo.pwr_state = 0;

	return OK;
}

static int broadcast_Isdb_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int rc = 0;

	int addr;

	print_log(NULL, "broadcast_Isdb_i2c_probe client:0x%lX\n", (UDynamic_32_64)client);
	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		print_log(NULL, "need I2C_FUNC_I2C\n");
		return -ENODEV;
	}
	/* taew00k.kang added for Device Tree Structure 2013-06-04 [start] */
	addr = client->addr; //Slave Addr
	pr_err("[1seg] i2c Slaveaddr [%x] \n", addr);

	IsdbCtrlInfo.pclient = client;
	//i2c_set_clientdata(client, (void*)&IsdbCtrlInfo.pclient);

	tunerbb_drv_hw_setting();

#if defined(USE_PM8941_XO_A2)
    IsdbCtrlInfo.pm8941_xo_a2_192000_clk = clk_get(&client->dev , "xo");
    if ( IS_ERR(IsdbCtrlInfo.pm8941_xo_a2_192000_clk) )
    {
        int ret = 0;

        ret = PTR_ERR(IsdbCtrlInfo.pm8941_xo_a2_192000_clk);
        dev_err(&client->dev, "[1seg] fc8300_i2c_probe clk_get error!!!");
    }
#endif

#ifndef _NOT_USE_WAKE_LOCK_
	wake_lock_init(&IsdbCtrlInfo.wake_lock, WAKE_LOCK_SUSPEND,
					dev_name(&client->dev));	
#endif

	return rc;
}

static int broadcast_Isdb_i2c_remove(struct i2c_client* client)
{
	int rc = 0;

	print_log(NULL, "[%s]\n", __func__);
#ifndef _NOT_USE_WAKE_LOCK_
	wake_lock_destroy(&IsdbCtrlInfo.wake_lock);
#endif
	memset((unsigned char*)&IsdbCtrlInfo, 0x0, sizeof(struct broadcast_fc8300_ctrl_data));
	//TcpalDeleteSemaphore(&fc8300DrvSem);
	return rc;
}

static int broadcast_Isdb_i2c_suspend(struct i2c_client* client, pm_message_t mesg)
{
	int rc = 0;
	print_log(NULL, "[%s]\n", __func__);
	return rc;
}

static int broadcast_Isdb_i2c_resume(struct i2c_client* client)
{
	int rc = 0;
	print_log(NULL, "[%s]\n", __func__);
	return rc;
}

static const struct i2c_device_id isdbt_fc8300_id[] = {
/* taew00k.kang added for Device Tree Structure 2013-06-04 [start] */
	{"tcc3535_i2c",	0},
/* taew00k.kang added for Device Tree Structure 2013-06-04 [end] */
	{},
};

MODULE_DEVICE_TABLE(i2c, isdbt_fc8300_id);


/* taew00k.kang added for Device Tree Structure 2013-06-04 [start] */
static struct of_device_id tcc3535_i2c_table[] = {
{ .compatible = "telechips,tcc3535-i2c",}, //Compatible node must match dts
{ },
};
/* taew00k.kang added for Device Tree Structure 2013-06-04 [end] */

static struct i2c_driver broadcast_Isdb_driver = {
	.driver = {
		.name = "tcc3535_i2c",
		.owner = THIS_MODULE,
		.of_match_table = tcc3535_i2c_table,
	},
	.probe = broadcast_Isdb_i2c_probe,
	.remove	= broadcast_Isdb_i2c_remove,
	.id_table = isdbt_fc8300_id,
	.suspend = broadcast_Isdb_i2c_suspend,
	.resume  = broadcast_Isdb_i2c_resume,
};

int broadcast_dmb_drv_init(void)
{
	int rc;
	print_log(NULL, "[%s]\n", __func__);
	rc = broadcast_dmb_drv_start();	
	if (rc) 
	{
		print_log(NULL, "failed to load\n");
		return rc;
	}
	print_log(NULL, "[%s add i2c driver]\n", __func__);
	rc = i2c_add_driver(&broadcast_Isdb_driver);
	print_log(NULL, "broadcast_add_driver rc = (%d)\n", rc);
	return rc;
}

static void __exit broadcast_dmb_drv_exit(void)
{
	i2c_del_driver(&broadcast_Isdb_driver);
}

module_init(broadcast_dmb_drv_init);
module_exit(broadcast_dmb_drv_exit);
MODULE_DESCRIPTION("broadcast_dmb_drv_init");
MODULE_LICENSE("FCI");
