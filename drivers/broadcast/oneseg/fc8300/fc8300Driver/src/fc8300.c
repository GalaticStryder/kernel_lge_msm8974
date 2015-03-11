#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/module.h>
#include <soc/qcom/lge/board_lge.h>

#include "fc8300.h"
#include "bbm.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8300_regs.h"
#include "fc8300_isr.h"
#include "fci_hal.h"

#define INT_THR_SIZE (128*1024)
#define USE_PM8941_XO_A2

#include <linux/wakelock.h>
#ifdef USE_PM8941_XO_A2
#include <linux/clk.h>
struct clk *clk;
u8 use_pm8941_xo_a2_192000 = 0;
#endif /* USE_PM8941_XO_A2 */

#if defined(CONFIG_MACH_MSM8974_G2_KDDI)
static u8 is_g2_kddi = 1;
#else
static u8 is_g2_kddi = 0;
#endif

struct ISDBT_INIT_INFO_T *hInit;

u32 totalTS=0;
u32 totalErrTS=0;
unsigned char ch_num = 0;
u32 remain_ts_len=0;
u8 remain_ts_buf[INT_THR_SIZE];
u8 scan_mode;
u8 dm_en;

enum ISDBT_MODE driver_mode = ISDBT_POWEROFF;

int isdbt_open (struct inode *inode, struct file *filp);
long isdbt_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
int isdbt_release (struct inode *inode, struct file *filp);
Dynamic_32_64 isdbt_read(struct file *filp, char *buf, Dynamic_32_64 count, loff_t *f_pos);

struct wake_lock oneseg_wakelock;

#ifndef BBM_I2C_TSIF
static wait_queue_head_t isdbt_isr_wait;
#endif

//GPIO(RESET & INTRRUPT) Setting
//#define FC8300_NAME		"isdbt"
#define FC8300_NAME		"broadcast1"
//#define FC8300_NAME     "broadcast_isdbt"

#define RING_BUFFER_SIZE	(188 * 320 * 50)
#define GPIO_ISDBT_IRQ 77
#define GPIO_ISDBT_PWR_EN 76
#define GPIO_ISDBT_RST 75
#ifndef BBM_I2C_TSIF
u8 static_ringbuffer[RING_BUFFER_SIZE];
#endif
static DEFINE_MUTEX(ringbuffer_lock);

void isdbt_hw_setting(void)
{
	gpio_request(GPIO_ISDBT_PWR_EN, "ISDBT_EN");
	udelay(50);
	gpio_direction_output(GPIO_ISDBT_PWR_EN, 0);

#ifndef BBM_I2C_TSIF
	if(gpio_request(GPIO_ISDBT_IRQ, "ISDBT_IRQ_INT"))
		print_log(0,"ISDBT_IRQ_INT Port request error!!!\n");

	gpio_direction_input(GPIO_ISDBT_IRQ);
#endif

	gpio_request(GPIO_ISDBT_RST, "ISDBT_RST");
	udelay(50);
	gpio_direction_output(GPIO_ISDBT_RST, 0);
}

//POWER_ON & HW_RESET & INTERRUPT_CLEAR
void isdbt_hw_init(void)
{
	int i=0;

	while(driver_mode == ISDBT_DATAREAD)
	{
		msWait(100);
		if(i++>5)
			break;
	}

	print_log(0, "[FC8300]isdbt_hw_init \n");
	gpio_set_value(GPIO_ISDBT_RST, 0);
	gpio_set_value(GPIO_ISDBT_PWR_EN, 1);
	msWait(3);
	gpio_set_value(GPIO_ISDBT_RST, 1);
	msWait(2);

	driver_mode = ISDBT_POWERON;
	wake_lock(&oneseg_wakelock);
}

//POWER_OFF
void isdbt_hw_deinit(void)
{
	driver_mode = ISDBT_POWEROFF;
	gpio_set_value(GPIO_ISDBT_PWR_EN, 0);
	print_log(0, "[FC8300]isdbt_hw_deinit \n");

	wake_unlock(&oneseg_wakelock);
#ifdef USE_PM8941_XO_A2
	if (use_pm8941_xo_a2_192000 == 1) {
		if ( !IS_ERR_OR_NULL(clk) )
		{
			clk_disable_unprepare(clk);
		}
	}
#endif /* USE_PM8941_XO_A2 */
}

#ifndef BBM_I2C_TSIF
u8 irq_error_cnt;
static u8 isdbt_isr_sig=0;
static struct task_struct *isdbt_kthread = NULL;

static irqreturn_t isdbt_irq(int irq, void *dev_id)
{
	if(driver_mode == ISDBT_POWEROFF) {
		//print_log(0, "fc8300 isdbt_irq : abnormal Interrupt occurred fc8300 power off state.cnt : %d\n", irq_error_cnt);
		irq_error_cnt++;
	}
	else {
		isdbt_isr_sig++;
		wake_up(&isdbt_isr_wait);
	}
	return IRQ_HANDLED;
}

int data_callback(u32 hDevice, u8 bufid, u8 *data, int len)
{
	struct ISDBT_INIT_INFO_T *hInit;
	struct list_head *temp;
	int i;

	totalTS +=(len/188);

	for(i=0;i<len;i+=188)
	{
		if((data[i+1]&0x80)||data[i]!=0x47)
			totalErrTS++;
	}

	hInit = (struct ISDBT_INIT_INFO_T *)hDevice;

	list_for_each(temp, &(hInit->hHead))
	{
		struct ISDBT_OPEN_INFO_T *hOpen;

		hOpen = list_entry(temp, struct ISDBT_OPEN_INFO_T, hList);

		if(hOpen->isdbttype == TS_TYPE)
		{
			mutex_lock(&ringbuffer_lock);
			if(fci_ringbuffer_free(&hOpen->RingBuffer) < len )
				FCI_RINGBUFFER_SKIP(&hOpen->RingBuffer, len);

			fci_ringbuffer_write(&hOpen->RingBuffer, data, len);
			wake_up_interruptible(&(hOpen->RingBuffer.queue));

			mutex_unlock(&ringbuffer_lock);
		}
	}

	return 0;
}

static int isdbt_thread(void *hDevice)
{
	struct ISDBT_INIT_INFO_T *hInit = (struct ISDBT_INIT_INFO_T *)hDevice;

	set_user_nice(current, -20);

	print_log(hInit, "isdbt_kthread enter\n");

	bbm_com_ts_callback_register((u32)hInit, data_callback);

	init_waitqueue_head(&isdbt_isr_wait);

	while(1)
	{
		wait_event_interruptible(isdbt_isr_wait, isdbt_isr_sig || kthread_should_stop());
		if (irq_error_cnt >= 1){
			print_log(0, "fc8300 isdbt_irq : abnormal Interrupt occurred fc8300 power off state.cnt : %d\n", irq_error_cnt);
			irq_error_cnt = 0;
		}
		if(driver_mode == ISDBT_POWERON)
		{
			driver_mode = ISDBT_DATAREAD;
			bbm_com_isr(hInit);
			driver_mode = ISDBT_POWERON;
		}

		if(isdbt_isr_sig>0)
		{
			isdbt_isr_sig--;
		}

		if (kthread_should_stop())
			break;
	}

	bbm_com_ts_callback_deregister();

	print_log(hInit, "isdbt_kthread exit\n");

	return 0;
}
#endif

static struct file_operations isdbt_fops =
{
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= isdbt_ioctl,
	.open		= isdbt_open,
	.read		= isdbt_read,
	.release	= isdbt_release,
};

static struct miscdevice fc8300_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = FC8300_NAME,
    .fops = &isdbt_fops,
};

int isdbt_open (struct inode *inode, struct file *filp)
{
	struct ISDBT_OPEN_INFO_T *hOpen;

	print_log(hInit, "isdbt open\n");

	hOpen = (struct ISDBT_OPEN_INFO_T *)kmalloc(sizeof(struct ISDBT_OPEN_INFO_T), GFP_KERNEL);
#ifndef BBM_I2C_TSIF
	hOpen->buf = &static_ringbuffer[0];
#endif
	hOpen->isdbttype = 0;

	list_add(&(hOpen->hList), &(hInit->hHead));

	hOpen->hInit = (HANDLE *)hInit;
#ifndef BBM_I2C_TSIF
	if(hOpen->buf == NULL)
	{
		print_log(hInit, "ring buffer malloc error\n");
		return -ENOMEM;
	}
	fci_ringbuffer_init(&hOpen->RingBuffer, hOpen->buf, RING_BUFFER_SIZE);
#endif

	filp->private_data = hOpen;

	return 0;
}

 Dynamic_32_64 isdbt_read(struct file *filp, char *buf, Dynamic_32_64 count, loff_t *f_pos)
{
    s32 avail;
    s32 non_blocking = filp->f_flags & O_NONBLOCK;
    struct ISDBT_OPEN_INFO_T *hOpen = (struct ISDBT_OPEN_INFO_T*)filp->private_data;
    struct fci_ringbuffer *cibuf = &hOpen->RingBuffer;
    Dynamic_32_64 len, read_len = 0;

    if (!cibuf->data || !count)
    {
        //print_log(hInit, " return 0\n");
        return 0;
    }

    if (non_blocking && (fci_ringbuffer_empty(cibuf)))
    {
        //print_log(hInit, "return EWOULDBLOCK\n");
        return -EWOULDBLOCK;
    }

	if (wait_event_interruptible(cibuf->queue,
		!fci_ringbuffer_empty(cibuf))) {
		print_log(hInit, "return ERESTARTSYS\n");
		return -ERESTARTSYS;
	}

	mutex_lock(&ringbuffer_lock);

    avail = fci_ringbuffer_avail(cibuf);

	if (count >= avail)
		len = avail;
	else
		len = count - (count % 188);

	read_len = fci_ringbuffer_read_user(cibuf, buf, len);

	mutex_unlock(&ringbuffer_lock);

	return read_len;
}

static  Dynamic_32_64 ioctl_isdbt_read(struct ISDBT_OPEN_INFO_T *hOpen  ,void __user *arg)
{
	struct broadcast_dmb_data_info __user* puserdata = (struct broadcast_dmb_data_info  __user*)arg;
	int ret = -ENODEV;
	Dynamic_32_64 count;
	DMB_BB_HEADER_TYPE dmb_header;
	static int read_count = 0;
	char *buf;

	s32 avail;
	struct fci_ringbuffer *cibuf = &hOpen->RingBuffer;
	Dynamic_32_64 len, total_len = 0;

	buf = puserdata->data_buf + sizeof(DMB_BB_HEADER_TYPE);
	count = puserdata->data_buf_size - sizeof(DMB_BB_HEADER_TYPE);
	count = (count/188)*188;

    if (!cibuf->data || !count)
    {
        print_log(hInit, " ioctl_isdbt_read return 0\n");
        return 0;
    }

    if ( fci_ringbuffer_empty(cibuf) )
    {
        //print_log(hInit, "return fci_ringbuffer_empty EWOULDBLOCK\n");
        return -EWOULDBLOCK;
    }

	mutex_lock(&ringbuffer_lock);
    avail = fci_ringbuffer_avail(cibuf);

	if (count >= avail)
		len = avail;
	else
		len = count - (count % 188);

	total_len = fci_ringbuffer_read_user(cibuf, buf, len);
	mutex_unlock(&ringbuffer_lock);

	dmb_header.data_type = DMB_BB_DATA_TS;
	dmb_header.size = (unsigned short)total_len;
	dmb_header.subch_id = ch_num;//0xFF;
	dmb_header.reserved = read_count++;

	ret = copy_to_user(puserdata->data_buf, &dmb_header, sizeof(DMB_BB_HEADER_TYPE));

	puserdata->copied_size = total_len + sizeof(DMB_BB_HEADER_TYPE);

	return ret;
}

int isdbt_release (struct inode *inode, struct file *filp)
{
	struct ISDBT_OPEN_INFO_T *hOpen;

	hOpen = filp->private_data;

	hOpen->isdbttype = 0;

	list_del(&(hOpen->hList));
	kfree(hOpen);

	return 0;
}

int fc8300_if_test(void)
{
	int res=0;
	int i;
	u16 wdata=0;
	u32 ldata=0;
	u8 data=0;
	u8 temp = 0;

	print_log(0, "fc8300_if_test Start!!!\n");
	for(i=0;i<100;i++) {
		bbm_com_byte_write(0, DIV_BROADCAST, 0xa4, i&0xff);
		bbm_com_byte_read(0, DIV_BROADCAST, 0xa4, &data);
		if((i&0xff) != data) {
			print_log(0, "fc8300_if_btest!   i=0x%x, data=0x%x\n", i&0xff, data);
			res=1;
		}
	}

	for(i = 0 ; i < 100 ; i++) {
		bbm_com_word_write(0, DIV_BROADCAST, 0xa4, i&0xffff);
		bbm_com_word_read(0, DIV_BROADCAST, 0xa4, &wdata);
		if((i & 0xffff) != wdata) {
			print_log(0, "fc8300_if_wtest!   i=0x%x, data=0x%x\n", i&0xffff, wdata);
			res = 1;
		}
	}

	for(i = 0 ; i < 100; i++) {
		bbm_com_long_write(0, DIV_BROADCAST, 0xa4, i&0xffffffff);
		bbm_com_long_read(0, DIV_BROADCAST, 0xa4, &ldata);
		if((i&0xffffffff) != ldata) {
			print_log(0, "fc8300_if_ltest!   i=0x%x, data=0x%x\n", i&0xffffffff, ldata);
			res=1;
		}
	}

	for(i=0 ; i < 100 ; i++) {
		temp = i & 0xff;
		bbm_com_tuner_write(NULL, DIV_BROADCAST, 0x7a, 0x01, &temp, 0x01);
		bbm_com_tuner_read(NULL, DIV_BROADCAST, 0x7a, 0x01, &data, 0x01);
		if((i & 0xff) != data)
			print_log(0, "FC8300 tuner test (0x%x,0x%x)\n", i & 0xff, data);
	}

	print_log(0, "fc8300_if_test End!!!\n");

	return res;
}

void isdbt_get_signal_info(HANDLE hDevice, u16 *Lock, u16 *CN
	, u32 *ui32BER_A, u32 *ui32PER_A, u32 *ui32BER_B, u32 *ui32PER_B, u32 *ui32BER_C, u32 *ui32PER_C
	, s32 *i32RSSI)
{
	u8	mod_info, fd;
	s32 res;

	struct dm_st {
		u8	start;
		s8	rssi;
		u8	sync_0;
		u8	sync_1;

		u8	fec_on;
		u8	fec_layer;
		u8	wscn;
		u8	reserved;

		u16 vit_a_ber_rxd_rsps;
		u16 vit_a_ber_err_rsps;
		u32 vit_a_ber_err_bits;

		u16 vit_b_ber_rxd_rsps;
		u16 vit_b_ber_err_rsps;
		u32 vit_b_ber_err_bits;

		u16 vit_c_ber_rxd_rsps;
		u16 vit_c_ber_err_rsps;
		u32 vit_c_ber_err_bits;

		u16 reserved0;
		u16 reserved1;
		u32 reserved2;

		u32 dmp_a_ber_rxd_bits;
		u32 dmp_a_ber_err_bits;

		u32 dmp_b_ber_rxd_bits;
		u32 dmp_b_ber_err_bits;

		u32 dmp_c_ber_rxd_bits;
		u32 dmp_c_ber_err_bits;

		u32 reserved3;
		u32 reserved4;
	} dm;

	res = bbm_com_bulk_read(hDevice, DIV_BROADCAST, BBM_DM_DATA, (u8*) &dm, sizeof(dm));

	if(res)
		print_log(NULL, "mtv_signal_measure Error res : %d\n");

	if(dm.sync_1 & 0x02)
		*Lock = 1;
	else
		*Lock =0;

	if (dm.vit_a_ber_rxd_rsps)
		*ui32PER_A = ((u32)dm.vit_a_ber_err_rsps * 10000 / (u32) dm.vit_a_ber_rxd_rsps);
	else
		*ui32PER_A = 10000;

	if (dm.vit_b_ber_rxd_rsps)
		*ui32PER_B = ((u32) dm.vit_b_ber_err_rsps * 10000 / (u32) dm.vit_b_ber_rxd_rsps);
	else
		*ui32PER_B = 10000;

	if (dm.vit_c_ber_rxd_rsps)
		*ui32PER_C = ((u32) dm.vit_c_ber_err_rsps * 10000 / (u32) dm.vit_c_ber_rxd_rsps);
	else
		*ui32PER_C = 10000;

	if (dm.dmp_a_ber_rxd_bits)
		*ui32BER_A = ((u32) dm.dmp_a_ber_err_bits * 10000 / (u32) dm.dmp_a_ber_rxd_bits);
	else
		*ui32BER_A = 10000;

	if (dm.dmp_b_ber_rxd_bits)
		*ui32BER_B = ((u32) dm.dmp_b_ber_err_bits * 10000 / (u32) dm.dmp_b_ber_rxd_bits);
	else
		*ui32BER_B = 10000;

	if (dm.dmp_c_ber_rxd_bits)
		*ui32BER_C = ((u32) dm.dmp_c_ber_err_bits * 10000 / (u32) dm.dmp_c_ber_rxd_bits);
	else
		*ui32BER_C = 10000;

	*i32RSSI = (signed char) dm.rssi;

	/* WSCN 		 */
	bbm_com_read(hDevice, DIV_BROADCAST, 0x4113, &mod_info);

	mod_info = mod_info & 0x70;

	bbm_com_read(hDevice, DIV_BROADCAST, 0x4066, &fd);

	if (fd < 50) {
		if (mod_info == 0x40) { /* QPSK */
			if (dm.wscn <= 2)
				dm.wscn = 0;
			else if (dm.wscn == 3)
				dm.wscn = dm.wscn - 2;
			else if (dm.wscn == 4)
				dm.wscn = dm.wscn - 1;
		}
		else if (mod_info == 0x20) { /* 16QAM */
			if (dm.wscn >= 0 && dm.wscn <= 4)
				dm.wscn = 0;
			else if (dm.wscn >= 5 && dm.wscn <= 8)
				dm.wscn = dm.wscn - 5;
			else if (dm.wscn == 9)
				dm.wscn = dm.wscn - 4;
			else if (dm.wscn >= 10 && dm.wscn <=11)
				dm.wscn = dm.wscn - 3;
			else if (dm.wscn == 12)
				dm.wscn = dm.wscn - 2;
			else if (dm.wscn == 13)
				dm.wscn = dm.wscn - 1;
		}
	}
	else if (fd < 90) {
		if (mod_info == 0x40) {/* QPSK */
			if (dm.wscn <=	2)
				dm.wscn = 0;
			else if (dm.wscn ==  3)
				dm.wscn = dm.wscn - 2;
			else if (dm.wscn ==  4)
				dm.wscn = dm.wscn - 1;
			else if (dm.wscn <= 8)
				dm.wscn = dm.wscn + 1;
			else if (dm.wscn ==  9)
				dm.wscn = dm.wscn + 2;
			else if (dm.wscn == 10)
				dm.wscn = dm.wscn + 3;
			else if (dm.wscn == 11)
				dm.wscn = dm.wscn + 4;
			else if (dm.wscn == 12)
				dm.wscn = dm.wscn + 2;
			else if (dm.wscn == 13)
				dm.wscn = dm.wscn + 3;
			else if (dm.wscn >= 14)
				dm.wscn = dm.wscn + 5;
		}
		else if (mod_info == 0x20) { /* 16QAM */
			if (dm.wscn >= 0 && dm.wscn <= 4)
				dm.wscn = 0;
			else if (dm.wscn >= 5 && dm.wscn <= 7)
				dm.wscn = dm.wscn - 4;
			else if (dm.wscn ==  8)
				dm.wscn = dm.wscn - 3;
			else if (dm.wscn ==  9)
				dm.wscn = dm.wscn - 2;
			else if (dm.wscn <= 12)
				dm.wscn = dm.wscn + 1;
			else if (dm.wscn == 13)
				dm.wscn = dm.wscn + 2;
			else if (dm.wscn == 14)
				dm.wscn = dm.wscn + 2;
			else if (dm.wscn >= 15)
				dm.wscn = dm.wscn + 3;
		}
	}
	else {
		if (mod_info == 0x40) {/* QPSK */
			if (dm.wscn <=	2)
				dm.wscn = 0;
			else if (dm.wscn == 3)
				dm.wscn = dm.wscn - 2;
			else if (dm.wscn == 4)
				dm.wscn = dm.wscn - 1;
			else if (dm.wscn <= 7)
				dm.wscn = dm.wscn + 1;
			else if (dm.wscn == 8)
				dm.wscn = dm.wscn + 2;
			else if (dm.wscn == 9)
				dm.wscn = dm.wscn + 3;
			else if (dm.wscn == 10)
				dm.wscn = dm.wscn + 5;
			else if (dm.wscn == 11)
				dm.wscn = dm.wscn + 3;
			else if (dm.wscn == 12)
				dm.wscn = dm.wscn + 4;
			else if (dm.wscn == 13)
				dm.wscn = dm.wscn + 5;
			else if (dm.wscn >= 14)
				dm.wscn = dm.wscn + 10;
		}
		else if (mod_info == 0x20) { /* 16QAM */
			if (dm.wscn >= 0 && dm.wscn <= 5)
				dm.wscn = 0;
			else if (dm.wscn == 6)
				dm.wscn = dm.wscn - 5;
			else if (dm.wscn == 7)
				dm.wscn = dm.wscn - 3;
			else if (dm.wscn == 8)
				dm.wscn = dm.wscn - 2;
			else if (dm.wscn == 9)
				dm.wscn = dm.wscn - 1;
			else if (dm.wscn <= 11)
				dm.wscn = dm.wscn + 2;
			else if (dm.wscn == 12)
				dm.wscn = dm.wscn + 4;
			else if (dm.wscn >= 13)
				dm.wscn = dm.wscn + 7;
		}
	}

	*CN = dm.wscn;

	//print_log(hDevice, "[FC8300]LOCK :%d, RSSI : %d, CN : %d
	//					, BER_A: %d, PER_A : %d, BER_B: %d, PER_B : %d, BER_C: %d, PER_C : %d\n"
	//					, *Lock, *i32RSSI, *CN,  *ui32BER_A, *ui32PER_A, *ui32BER_B, *ui32PER_B, *ui32BER_C, *ui32PER_C);
	return;
}

void isdbt_isr_check(HANDLE hDevice)
{
#ifndef BBM_I2C_TSIF
	u8 isr_time=0;
#endif

	bbm_com_write(hDevice, DIV_BROADCAST, BBM_BUF_INT_ENABLE, 0x00);

#ifndef BBM_I2C_TSIF
	while(isr_time < 10) {
		if(!isdbt_isr_sig) {
			break;
		}
		msWait(10);
		isr_time++;
	}
#endif
}

long isdbt_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	s32 res = BBM_NOK;

	void __user *argp = (void __user *)arg;

	s32 err = 0;
	s32 size = 0;
	int uData=0;
	struct ISDBT_OPEN_INFO_T *hOpen;

	IOCTL_ISDBT_SIGNAL_INFO isdbt_signal_info;

	if(_IOC_TYPE(cmd) != ISDBT_IOC_MAGIC)
	{
		return -EINVAL;
	}

	if(_IOC_NR(cmd) >= IOCTL_MAXNR)
	{
		return -EINVAL;
	}

	hOpen = filp->private_data;

	size = _IOC_SIZE(cmd);

	print_log(0, "[1seg] isdbt_ioctl  0x%x\n", cmd);

	switch(cmd)
	{
		case IOCTL_ISDBT_POWER_ON:
		case LGE_BROADCAST_DMB_IOCTL_ON:
			print_log(0, "[1seg] IOCTL_ISDBT_POWER_ON \n");

			isdbt_hw_init();
#ifdef USE_PM8941_XO_A2
			if (use_pm8941_xo_a2_192000 == 1) {
				if ( !IS_ERR_OR_NULL(clk) )
				{
					int ret = -1;
					ret = clk_prepare_enable(clk);
					if (ret) {
						print_log(0,"[1seg] LGE_BROADCAST_DMB_IOCTL_ON enable clock error!!!\n");
						return -1;
					}
				}
			}
#endif /* USE_PM8941_XO_A2 */
			res = bbm_com_i2c_init(hInit, FCI_HPI_TYPE);
			print_log(hInit, "[1seg] FC8300 BBM_I2C_INIT res : %d \n", res);

			res |= bbm_com_probe(hInit, DIV_BROADCAST);
			print_log(hInit, "[1seg] FC8300 BBM_PROBE res : %d \n", res);

			if(res) {
				print_log(hInit, "[1seg] FC8300 Initialize Fail : %d \n", res);
			//	break;
			}

			res |= bbm_com_init(hInit, DIV_BROADCAST);
			res |= bbm_com_tuner_select(hInit, DIV_BROADCAST, FC8300_TUNER, ISDBT_13SEG);
			scan_mode = 0;
			dm_en = 0;

			if(res)
			print_log(0, "[1seg] IOCTL_ISDBT_POWER_ON FAIL \n");
			else
			print_log(0, "[1seg] IOCTL_ISDBT_POWER_OK \n");

			//fc8300_if_test();
			break;
		case IOCTL_ISDBT_POWER_OFF:
		case LGE_BROADCAST_DMB_IOCTL_OFF:

			print_log(0, "IOCTL_ISDBT_POWER_OFF \n");
			isdbt_hw_deinit();
			res = BBM_OK;
			break;
		case IOCTL_ISDBT_SCAN_FREQ:
		{
			u32 f_rf;
			err = copy_from_user((void *)&uData, (void *)arg, size);

			f_rf = (uData- 13) * 6000 + 473143;
			//print_log(0, "IOCTL_ISDBT_SCAN_FREQ  f_rf : %d\n", f_rf);

			isdbt_isr_check(hInit);
			res = bbm_com_tuner_set_freq(hInit, DIV_BROADCAST, f_rf, 0x15);
			bbm_com_write(hInit, DIV_BROADCAST, BBM_BUF_INT_ENABLE, 0x01);
			res |= bbm_com_scan_status(hInit, DIV_BROADCAST);
		}
			break;
		case IOCTL_ISDBT_SET_FREQ:
		{
			u32 f_rf;
			totalTS=0;
			totalErrTS=0;
			remain_ts_len=0;

			err = copy_from_user((void *)&uData, (void *)arg, size);
#ifndef BBM_I2C_TSIF
			mutex_lock(&ringbuffer_lock);
			fci_ringbuffer_flush(&hOpen->RingBuffer);
			mutex_unlock(&ringbuffer_lock);
#endif
			f_rf = (uData- 13) * 6000 + 473143;
			//print_log(0, "IOCTL_ISDBT_SET_FREQ chNum : %d, f_rf : %d\n", uData, f_rf);

			isdbt_isr_check(hInit);
			res = bbm_com_tuner_set_freq(hInit, DIV_BROADCAST, f_rf, 0x15);
			bbm_com_write(hInit, DIV_BROADCAST, BBM_BUF_INT_ENABLE, 0x01);
			res |= bbm_com_scan_status(hInit, DIV_BROADCAST);
		}
			break;
		case IOCTL_ISDBT_GET_LOCK_STATUS:
			{
				u8 data;
				print_log(0, "IOCTL_ISDBT_GET_LOCK_STATUS \n");
				bbm_com_read(hInit, DIV_BROADCAST, 0x3026, &data);

				if(data & 0x02)
					uData=1;
				else
					uData=0;
				err |= copy_to_user((void *)arg, (void *)&uData, size);
				res = BBM_OK;
			}
			break;
		case IOCTL_ISDBT_GET_SIGNAL_INFO:
			isdbt_get_signal_info(hInit, &isdbt_signal_info.lock, &isdbt_signal_info.cn
				, &isdbt_signal_info.ber_A, &isdbt_signal_info.per_A
				, &isdbt_signal_info.ber_B, &isdbt_signal_info.per_B
				, &isdbt_signal_info.ber_C, &isdbt_signal_info.per_C
				, &isdbt_signal_info.rssi);

			isdbt_signal_info.ErrTSP = totalErrTS;
			isdbt_signal_info.TotalTSP = totalTS;

			totalTS=totalErrTS=0;

			err |= copy_to_user((void *)arg, (void *)&isdbt_signal_info, size);

			res = BBM_OK;

			break;
		case IOCTL_ISDBT_START_TS:
			hOpen->isdbttype = TS_TYPE;
			res = BBM_OK;
			break;
		case IOCTL_ISDBT_STOP_TS:
		case LGE_BROADCAST_DMB_IOCTL_USER_STOP:
			hOpen->isdbttype = 0;
			res = BBM_OK;
			break;

		case LGE_BROADCAST_DMB_IOCTL_SET_CH:
			{
				struct broadcast_dmb_set_ch_info udata;
				u32 f_rf;
				//                                                  

				if(copy_from_user(&udata, argp, sizeof(struct broadcast_dmb_set_ch_info)))
				{
					print_log(0,"broadcast_dmb_set_ch fail!!! \n");
					res = -1;
				}
				else
				{
					f_rf = (udata.ch_num- 13) * 6000 + 473143;
					//print_log(0, "IOCTL_ISDBT_SET_FREQ freq:%d, RF:%d\n",udata.ch_num,f_rf);
					isdbt_isr_check(hInit);
					res = bbm_com_tuner_set_freq(hInit, DIV_BROADCAST, f_rf, 0x15);
					bbm_com_write(hInit, DIV_BROADCAST, BBM_BUF_INT_ENABLE, 0x01);

					if(udata.mode == LGE_BROADCAST_OPMODE_ENSQUERY)
					{
						res |= bbm_com_scan_status(hInit, DIV_BROADCAST);
						if(res != BBM_OK)
						{
							print_log(0, " BBM_SCAN_STATUS  Unlock \n");
							break;
						}
						print_log(0, " BBM_SCAN_STATUS : Lock \n");
					}

					// print_log(0, "IOCTL_ISDBT_SET_FREQ \n");
					totalTS=0;
					totalErrTS=0;
					remain_ts_len=0;
					ch_num = udata.ch_num;
#ifndef BBM_I2C_TSIF
					mutex_lock(&ringbuffer_lock);
					fci_ringbuffer_flush(&hOpen->RingBuffer);
					mutex_unlock(&ringbuffer_lock);
#endif
					hOpen->isdbttype = TS_TYPE;
				}
			}
			break;
		case LGE_BROADCAST_DMB_IOCTL_GET_SIG_INFO:
			{
				struct broadcast_dmb_sig_info udata;
				//                                                        

				isdbt_get_signal_info(hInit, &isdbt_signal_info.lock, &isdbt_signal_info.cn
					, &isdbt_signal_info.ber_A, &isdbt_signal_info.per_A
					, &isdbt_signal_info.ber_B, &isdbt_signal_info.per_B
					, &isdbt_signal_info.ber_C, &isdbt_signal_info.per_C
					, &isdbt_signal_info.rssi);

				isdbt_signal_info.ErrTSP = totalErrTS;
				isdbt_signal_info.TotalTSP = totalTS;

				totalTS=totalErrTS=0;

				udata.info.oneseg_info.lock = (int)isdbt_signal_info.lock;
				udata.info.oneseg_info.ErrTSP = (int)isdbt_signal_info.ErrTSP;
				udata.info.oneseg_info.TotalTSP = (int)isdbt_signal_info.TotalTSP;

				udata.info.oneseg_info.ber_A = (int)isdbt_signal_info.ber_A;
				udata.info.oneseg_info.per_A = (int)isdbt_signal_info.per_A;
				udata.info.oneseg_info.ber_B = (int)isdbt_signal_info.ber_B;
				udata.info.oneseg_info.per_B = (int)isdbt_signal_info.per_B;
				udata.info.oneseg_info.ber_C = (int)isdbt_signal_info.ber_C;
				udata.info.oneseg_info.per_C = (int)isdbt_signal_info.per_C;

				udata.info.oneseg_info.rssi = (int)isdbt_signal_info.rssi;
				udata.info.oneseg_info.cn = (int)isdbt_signal_info.cn;
                udata.info.oneseg_info.antenna_level = 0;

				if(copy_to_user((void *)argp, &udata, sizeof(struct broadcast_dmb_sig_info)))
				{
					print_log(0,"broadcast_dmb_get_sig_info copy_to_user error!!! \n");
					res = BBM_NOK;
				}
				else
				{
                    print_log(0, "LOCK :%d, RSSI : %d, CN : %d, BER_A: %d, PER_A : %d, BER_B: %d, PER_B : %d, BER_C: %d, PER_C : %d\n",
                        udata.info.oneseg_info.lock,
                        udata.info.oneseg_info.rssi,
                        udata.info.oneseg_info.cn,
                        udata.info.oneseg_info.ber_A,
                        udata.info.oneseg_info.per_A,
                        udata.info.oneseg_info.ber_B,
                        udata.info.oneseg_info.per_B,
                        udata.info.oneseg_info.ber_C,
                        udata.info.oneseg_info.per_C);

					res = BBM_OK;
				}
			}
			break;

		case LGE_BROADCAST_DMB_IOCTL_GET_DMB_DATA:
			//                                                        
			res = ioctl_isdbt_read(hOpen,argp);
			break;
		case LGE_BROADCAST_DMB_IOCTL_OPEN:
		case LGE_BROADCAST_DMB_IOCTL_CLOSE:
		case LGE_BROADCAST_DMB_IOCTL_RESYNC:
		case LGE_BROADCAST_DMB_IOCTL_DETECT_SYNC:
		case LGE_BROADCAST_DMB_IOCTL_GET_CH_INFO:
		case LGE_BROADCAST_DMB_IOCTL_RESET_CH:
		case LGE_BROADCAST_DMB_IOCTL_SELECT_ANTENNA:
			print_log(0, "LGE_BROADCAST_DMB_IOCTL_SKIP \n");
            res = BBM_OK;
			break;
		default:
			print_log(hInit, "isdbt ioctl error!\n");
			res = BBM_NOK;
			break;
	}

	if(err < 0)
	{
		print_log(hInit, "copy to/from user fail : %d", err);
		res = BBM_NOK;
	}
	return res;
}

int isdbt_init(void)
{
	s32 res;

	print_log(hInit, "isdbt_init 20130619\n");

	res = misc_register(&fc8300_misc_device);

	if(res < 0)
	{
		print_log(hInit, "isdbt init fail : %d\n", res);
		return res;
	}
#ifdef USE_PM8941_XO_A2

		if (lge_get_board_revno() >= HW_REV_D  && (is_g2_kddi == 0) ) {
			use_pm8941_xo_a2_192000 = 1;
			print_log(hInit, "[1seg] A1-DCM rev.D or later version: %d\n",use_pm8941_xo_a2_192000);
		}
		else {
			use_pm8941_xo_a2_192000 = 0;
			print_log(hInit, "[1seg] A1-DCM rev.C : %d\n",use_pm8941_xo_a2_192000);
		}
#endif /* USE_PM8941_XO_A2 */
	wake_lock_init(&oneseg_wakelock, WAKE_LOCK_SUSPEND, fc8300_misc_device.name);

	isdbt_hw_setting();

	hInit = (struct ISDBT_INIT_INFO_T *)kmalloc(sizeof(struct ISDBT_INIT_INFO_T), GFP_KERNEL);

	res = bbm_com_hostif_select(hInit, BBM_I2C);

	if(res)
	{
		print_log(hInit, "isdbt host interface select fail!\n");
	}
		else {
#ifdef USE_PM8941_XO_A2
			if (use_pm8941_xo_a2_192000 == 1) {
				if ( !IS_ERR_OR_NULL(clk) )
				{
					int ret = -1;
					ret = clk_prepare_enable(clk);
					if (ret) {
						print_log(0,"[1seg] isdbt_init enable clock error!!!\n");
						return -1;
					}
				}
			}
#endif /* USE_PM8941_XO_A2 */
		}

#ifndef BBM_I2C_TSIF
	if (!isdbt_kthread)
	{
		print_log(hInit, "kthread run\n");
		isdbt_kthread = kthread_run(isdbt_thread, (void*)hInit, "isdbt_thread");
	}

	res = request_irq(gpio_to_irq(GPIO_ISDBT_IRQ), isdbt_irq, IRQF_DISABLED | IRQF_TRIGGER_FALLING, FC8300_NAME, NULL);

	if(res)
		print_log(hInit, "dmb rquest irq fail : %d\n", res);
#endif

	INIT_LIST_HEAD(&(hInit->hHead));

	return 0;
}

void isdbt_exit(void)
{
	print_log(hInit, "isdbt isdbt_exit \n");

#ifndef BBM_I2C_TSIF
	free_irq(GPIO_ISDBT_IRQ, NULL);

	if(isdbt_kthread)
	{
		kthread_stop(isdbt_kthread);
		isdbt_kthread = NULL;
	}
#endif

	bbm_com_hostif_deselect(hInit);

	isdbt_hw_deinit();

	misc_deregister(&fc8300_misc_device);

	kfree(hInit);
	wake_lock_destroy(&oneseg_wakelock);

}

module_init(isdbt_init);
module_exit(isdbt_exit);

//MODULE_LICENSE("Dual BSD/GPL");
MODULE_LICENSE("GPL v2");

