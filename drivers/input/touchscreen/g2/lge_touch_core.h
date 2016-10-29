/* include/linux/lge_touch_core.h
 *
 * Copyright (C) 2011 LGE.
 *
 * Author: yehan.ahn@lge.com, hyesung.shin@lge.com
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

#ifndef LGE_TOUCH_CORE_H
#define LGE_TOUCH_CORE_H

/* #define MT_PROTOCOL_A */
/* #define LGE_TOUCH_TIME_DEBUG */
#include <linux/earlysuspend.h>

#define I2C_SUSPEND_WORKAROUND 1
#define MAX_FINGER	10
#define MAX_BUTTON	4

#define CUST_G2_TOUCH

#if !defined(CONFIG_MACH_MSM8974_VU3_KR) && !defined(CONFIG_LGE_Z_TOUCHSCREEN)
#define A1_only
#define CHARGER_CONNECTED_REG	0x0D
#endif

#if defined(CONFIG_LGE_Z_TOUCHSCREEN)
#define Z_GLOVE_TOUCH_SUPPORT
#endif

#ifdef CONFIG_MACH_MSM8974_G2_KR
#define F12_2D_CTRL10 0x15
#define TUNING_VALUE_NOISE_FLOOR 4
#define TUNING_VALUE_MINIMUM_PEAK_AMPLITUDE 4
#define TUNING_VALUE_PEAK_MERGE_THRESHOLD 48
#define TUNING_VALUE_NOISE_PEAK_FILTER 60
#endif

#ifdef CUST_G2_TOUCH
#define REPORT_WAKEUP_GESTURE_ONLY_REG	0x19 /* offset 2  bit:1 */
#define WAKEUP_GESTURE_ENABEL_REG	0x1D /* bit:0 */
#define DOUBLE_TAP_AREA_REG	0x18
#define DOZE_INTERVAL_REG	0xF
#endif

#ifdef CUST_G2_TOUCH
#include <mach/board_lge.h>
lcd_maker_id get_panel_maker_id(void);
#define MINIMUM_PEAK_AMPLITUDE_REG    0x15
#endif
#if defined(A1_only)
#define DRUMMING_THRESH_N_DISTANCE_REG  0x15
#endif

#ifdef CUST_G2_TOUCH
#include <linux/mfd/pm8xxx/cradle.h>
#endif

#if defined(CONFIG_LGE_Z_TOUCHSCREEN)
#define SMALL_FINGER_AMPLITUDE_THRESHOLD_REG    0x17
#endif

#ifdef CONFIG_LGE_SECURITY_KNOCK_ON
#define MAX_POINT_SIZE_FOR_LPWG 12

struct point
{
    int x;
    int y;
};
#endif

struct touch_device_caps {
	u8		button_support;
	u16		y_button_boundary;
	u32		button_margin;		/* percentage % */
	u8		number_of_button;
	u32		button_name[MAX_BUTTON];
	u8		is_width_supported;
	u8		is_pressure_supported;
	u8		is_id_supported;
	u32		max_width;
	u32		max_pressure;
	u32		max_id;
	u32		x_max;
	u32		y_max;
	u32		lcd_x;
	u32		lcd_y;
};

struct touch_operation_role {
	u8		operation_mode;	/* interrupt = 1 , polling = 0; */
	u8		key_type;		/* none = 0, hard_touch_key = 1, virtual_key = 2 */
	u8		report_mode;
	u8		delta_pos_threshold;
	u8		orientation;	/* 0' = 0, 90' = 1, 180' = 2, 270' = 3 */
	u32		report_period;	/* ns */
	u32		booting_delay;	/* ms */
	u32		reset_delay;	/* ms */
	u8		suspend_pwr;
	u8		resume_pwr;
	int		jitter_filter_enable;	/* enable = 1, disable = 0 */
	int		jitter_curr_ratio;
	int		accuracy_filter_enable;	/* enable = 1, disable = 0 */
	int		ghost_finger_solution_enable;
	unsigned long	irqflags;
#ifdef CUST_G2_TOUCH
	int		ghost_detection_enable;
#endif
};

struct touch_power_module {
	u8		use_regulator;
	char	vdd[30];
	int		vdd_voltage;
	char	vio[30];
	int		vio_voltage;
	int		(*power)	(struct i2c_client *client, int on);
};

struct touch_platform_data {
	int	int_pin;
	int	reset_pin;
	char	maker[30];
	char	fw_version[11];
	struct touch_device_caps *caps;
	u8 num_caps;
	struct touch_operation_role *role;
	u8 num_role;
	struct touch_power_module *pwr;
	u8 num_pwr;
};

struct t_data {
	u16	id;
#ifdef CUST_G2_TOUCH
	u8	object;
#endif
	u16	x_position;
	u16	y_position;
	u16	width_major;
	u16	width_minor;
	u16	width_orientation;
	u16	pressure;
	u8	status;
};

struct b_data {
	u16	key_code;
	u16	state;
};

struct touch_data {
	u8		total_num;
	u8		prev_total_num;
	u8		state;
	u8		palm;
#ifdef CUST_G2_TOUCH
	u8		large_palm_status;
#endif
	struct t_data	curr_data[MAX_FINGER];
	struct t_data	prev_data[MAX_FINGER];
	struct b_data	curr_button;
	struct b_data	prev_button;
};

struct fw_upgrade_info {
	char		fw_path[256];
	u8			fw_force_upgrade;
	volatile u8	is_downloading;
};

enum {
	TOUCH_CHIP_UNKNOWN_REV = 0,
	TOUCH_CHIP_REV_A,
	TOUCH_CHIP_REV_B,
};

enum {
	TOUCH_VENDOR_LGIT = 0,
	TOUCH_VENDOR_TPK,
};

#if defined(CONFIG_MACH_MSM8974_G2_OPEN_COM) || defined(CONFIG_MACH_MSM8974_G2_OPT_AU)
enum {
	TOUCH_PANEL_UNKNOWN = 0,
	TOUCH_PANEL_G1F_LGIT,
	TOUCH_PANEL_G1F_SSUNTEL,
	TOUCH_PANEL_G2_LGIT,
};
#endif

#if defined(CONFIG_LGE_Z_TOUCHSCREEN)
enum {
	TOUCH_PANEL_BAR_PATTERN = 0,
	TOUCH_PANEL_H_PATTERN,
};
#endif

struct fw_set_info {
	int bootloader_fw_ver;
	u8	ic_chip_rev;
	u8	prev_touch_vendor;
	u8	curr_touch_vendor;
#if defined(CONFIG_LGE_Z_TOUCHSCREEN)
	u8	customer_family;
#endif
};

struct touch_fw_info {
	struct fw_upgrade_info	fw_upgrade;
	struct fw_set_info		fw_setting;
	u8		ic_fw_identifier[31];	/* String */
	u8		ic_fw_version[11];		/* String */
#ifdef CUST_G2_TOUCH
	u8		fw_force_rework;
	u8		syna_img_fw_version[5];
	u8		syna_img_fw_product_id[11];
#endif
};

struct rect {
	u16	left;
	u16	right;
	u16	top;
	u16	bottom;
};

struct section_info {
	struct rect	panel;
	struct rect button[MAX_BUTTON];
	struct rect button_cancel[MAX_BUTTON];
	u16 b_inner_width;
	u16 b_width;
	u16 b_margin;
	u16 b_height;
	u16 b_num;
	u16 b_name[MAX_BUTTON];
};

struct ghost_finger_ctrl {
	volatile u8	 stage;
#ifdef CUST_G2_TOUCH
	int	incoming_call;
#endif
	int probe;
	int count;
	int min_count;
	int max_count;
	int ghost_check_count;
	int saved_x;
	int saved_y;
	int saved_last_x;
	int saved_last_y;
	int max_moved;
	int max_pressure;
};

struct jitter_history_data {
	u16	x;
	u16	y;
	u16	pressure;
	int	delta_x;
	int	delta_y;
};

struct jitter_filter_info {
	int	id_mask;
	int	adjust_margin;
	struct jitter_history_data	his_data[10];
};

struct accuracy_history_data {
	u16	x;
	u16	y;
	u16	pressure;
	int	count;
	int	mod_x;
	int	mod_y;
	int	axis_x;
	int	axis_y;
};

struct accuracy_filter_info {
	int	ignore_pressure_gap;
	int	touch_max_count;
	int	delta_max;
	int	max_pressure;
	int	direction_count;
	int	time_to_max_pressure;
	u16	finish_filter;
	int	pen_pressure;
	int	prev_total_num;
	struct accuracy_history_data	his_data;
};

struct ghost_detect_init_data {
	u32 mask;
	u8	rebase_count;
	u8	ghost_detection;
	u8	ghost_detection_count;
	u8	button_press_count;
	u8	ta_reset;
	u8	finger_subtraction_check_count;
	u8	ta_debouncing_count;
	int in_edge_zone_id;
	u16 in_edge_zone_x_pos;
	u16 in_edge_zone_y_pos;
	u8	palm_num;
	u8	start_palm;
	u32	palm_press_count;
	struct t_data ts_prev_finger_press_data;
	struct t_data ts_prev_palm_finger_press_data;
};

struct ghost_detect_ctrl {
	u8	is_resume;
	struct ghost_detect_init_data init_data;
};

#ifdef CONFIG_LGE_SECURITY_KNOCK_ON
struct state_info
{
	atomic_t power_state;
	atomic_t interrupt_state;
	atomic_t upgrade_state;
	atomic_t ta_state;
	atomic_t temperature_state;
	atomic_t proximity_state;
	atomic_t hallic_state;
	atomic_t uevent_state;
};
#endif

struct lge_touch_data {
	void *h_touch;
#ifdef CONFIG_LGE_SECURITY_KNOCK_ON
	struct state_info       state;
#endif
	atomic_t		next_work;
	atomic_t		device_init;
	u8				work_sync_err_cnt;
	u8				ic_init_err_cnt;
	volatile int	curr_pwr_state;
	int				int_pin_state;
	struct i2c_client			*client;
	struct input_dev			*input_dev;
	struct hrtimer				timer;
	struct work_struct		work;
	struct delayed_work			work_init;
	struct delayed_work			work_touch_lock;
	struct work_struct		work_fw_upgrade;
#ifdef CUST_G2_TOUCH
	struct mutex			irq_work_mutex;
	struct delayed_work			work_f54;
	struct delayed_work			work_gesture_wakeup;
	struct delayed_work			work_thermal;
#endif
#ifdef I2C_SUSPEND_WORKAROUND
	struct delayed_work check_suspended_work;
#endif
#if defined(A1_only) || defined(CONFIG_LGE_Z_TOUCHSCREEN)
	struct delayed_work			work_ime_drumming;
#endif
#if defined(CONFIG_FB)
	struct notifier_block		fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend		early_suspend;
#endif
	struct touch_platform_data	*pdata;
	struct touch_data			ts_data;
	struct touch_fw_info		fw_info;
	struct section_info			st_info;
	struct kobject				lge_touch_kobj;
	struct ghost_finger_ctrl	gf_ctrl;
	struct ghost_detect_ctrl	gd_ctrl;
	struct jitter_filter_info	jitter_filter;
	struct accuracy_filter_info	accuracy_filter;
};

#ifdef CONFIG_LGE_SECURITY_KNOCK_ON
enum{
    TA_DISCONNECTED = 0,
    TA_CONNECTED,
};

enum{
    PROXIMITY_FAR = 0,
    PROXIMITY_NEAR,
};

enum{
    HALL_NONE = 0,
    HALL_COVERED,
};

enum{
    UEVENT_IDLE = 0,
    UEVENT_BUSY,
};


typedef enum error_type {
    NO_ERROR = 0,
    ERROR,
    IGNORE_EVENT,
    IGNORE_EVENT_BUT_SAVE_IT,
} err_t;
#endif

struct touch_device_driver {
	int		(*probe)		(struct lge_touch_data *lge_touch_ts);
#if defined(CONFIG_LGE_Z_TOUCHSCREEN)
	int		(*resolution)	(struct i2c_client *client);
#endif
	void	(*remove)		(struct i2c_client *client);
	int		(*init)			(struct i2c_client *client, struct touch_fw_info *info);
	int		(*data)			(struct i2c_client *client, struct touch_data *data);
	int		(*power)		(struct i2c_client *client, int power_ctrl);
	int		(*ic_ctrl)		(struct i2c_client *client, u8 code, u16 value);
	int	(*fw_upgrade)	(struct i2c_client *client, struct touch_fw_info *info);
#ifdef CONFIG_LGE_SECURITY_KNOCK_ON
	err_t	 	(*suspend) (struct i2c_client *client);
	err_t	 	(*resume) (struct i2c_client *client);
	err_t	 	(*lpwg) (struct i2c_client *client, u32 code, u32 value, struct point *data);
#endif
};

#ifdef CUST_G2_TOUCH
enum{
	NO_PALM = 0,
	PALM_PRESSED,
	PALM_MOVE,
	PALM_RELEASED,
};

enum{
	NO_OBJECT = 0,
	FINGER,
	STYLUS,
	PALM,
	UNCLASSIFIED,
	RESERVED,
	GLOVED_FINGER,
};
#endif

enum{
	POLLING_MODE = 0,
	INTERRUPT_MODE,
	HYBRIDE_MODE
};

enum{
	POWER_OFF = 0,
	POWER_ON,
	POWER_SLEEP,
	POWER_WAKE
};

enum{
	KEY_NONE = 0,
	TOUCH_HARD_KEY,
	TOUCH_SOFT_KEY,
	VIRTUAL_KEY,
};

enum{
	CONTINUOUS_REPORT_MODE = 0,
	REDUCED_REPORT_MODE,
};

enum{
	RESET_NONE = 0,
	SOFT_RESET,
	PIN_RESET,
	VDD_RESET,
};

enum{
	DOWNLOAD_COMPLETE = 0,
	UNDER_DOWNLOADING,
};

enum{
	OP_NULL = 0,
	OP_RELEASE,
	OP_SINGLE,
	OP_MULTI,
	OP_LOCK,
};

enum{
	KEY_NULL = 0,
	KEY_PANEL,
	KEY_BOUNDARY
};

enum{
	DO_NOT_ANYTHING = 0,
	ABS_PRESS,
	ABS_RELEASE,
	BUTTON_PRESS,
	BUTTON_RELEASE,
	BUTTON_CANCEL,
	TOUCH_BUTTON_LOCK,
	TOUCH_ABS_LOCK
};

enum{
	BUTTON_RELEASED	= 0,
	BUTTON_PRESSED	= 1,
	BUTTON_CANCLED	= 0xff,
};

enum{
	FINGER_RELEASED	= 0,
	FINGER_PRESSED	= 1,
};

enum{
	KEYGUARD_RESERVED,
	KEYGUARD_ENABLE,
};

#ifdef CUST_G2_TOUCH
enum{
	INCOMIMG_CALL_RESERVED,
	INCOMIMG_CALL_TOUCH,
};

#if defined(A1_only) || defined(CONFIG_LGE_Z_TOUCHSCREEN)
enum{
	IME_OFF,
	IME_ON,
};
#endif
enum{
	GHOST_NONE			= 0,
	GHOST_LONG_PRESS	= (1U << 0),	/* 1 */
	GHOST_FIRST_IRQ		= (1U << 1),	/* 2 */
	GHOST_CALL_STATE	= (1U << 2),	/* 4 */
	GHOST_TA_DEBOUNCE	= (1U << 3),	/* 8 */
	GHOST_PRESSURE		= (1U << 4),	/* 16 */
	GHOST_BUTTON		= (1U << 5),	/* 32 */
	GHOST_TA_RESET		= (1U << 6),	/* 64 */
	GHOST_EDGE_ZONE		= (1U << 7),	/* 128 */
};
#endif

enum{
	GHOST_STAGE_CLEAR = 0,
	GHOST_STAGE_1 = 1,
	GHOST_STAGE_2 = 2,
	GHOST_STAGE_3 = 4,
	GHOST_STAGE_4 = 8,
};

enum{
	BASELINE_OPEN = 0,
	BASELINE_FIX,
	BASELINE_REBASE,
};

enum{
	IC_CTRL_CODE_NONE = 0,
	IC_CTRL_BASELINE,
	IC_CTRL_READ,
	IC_CTRL_WRITE,
	IC_CTRL_RESET_CMD,
	IC_CTRL_REPORT_MODE,
#if defined(Z_GLOVE_TOUCH_SUPPORT)
	IC_CTRL_GLOVE_TOUCH,
#endif
#ifdef CUST_G2_TOUCH
	IC_CTRL_DOUBLE_TAP_WAKEUP_MODE,
#endif
};

/* For Error Handling
  *
  * DO_IF : execute 'do_work', and if the result is true, print 'error_log' and goto 'goto_error'.
  * DO_SAFE : execute 'do_work', and if the result is '< 0', print 'error_log' and goto 'goto_error'
  * ASSIGN : excute 'do_assign', and if the result is 'NULL', print 'error_log' and goto 'goto_error'
  * ERROR_IF : if the condition is true(ERROR), print 'string' and goto 'goto_error'.
  */
#define DO_IF(do_work, goto_error)                              \
do {                                                \
    if(do_work){                                        \
        printk(KERN_INFO "[Touch E] Action Failed [%s %d] \n", __FUNCTION__, __LINE__); \
        goto goto_error;                                \
    }                                           \
} while(0)

#define DO_SAFE(do_work, goto_error)                                \
    DO_IF(unlikely((do_work) < 0), goto_error)

#define ASSIGN(do_assign, goto_error)                               \
do {                                                \
    if((do_assign) == NULL){                                \
        printk(KERN_INFO "[Touch E] Assign Failed [%s %d] \n", __FUNCTION__, __LINE__); \
        goto goto_error;                                \
    }                                           \
} while(0)

#define ERROR_IF(cond, string, goto_error)  \
do {                        \
    if(cond){               \
        TOUCH_ERR_MSG(string);      \
        goto goto_error;        \
    }                   \
} while(0)

enum{
    NOTIFY_TA_CONNECTION = 1,
    NOTIFY_TEMPERATURE_CHANGE,
    NOTIFY_PROXIMITY,
    NOTIFY_HALL_IC,
};

enum{
    LPWG_NONE = 0,
    LPWG_DOUBLE_TAP,
    LPWG_PASSWORD,
};

#ifdef CONFIG_LGE_SECURITY_KNOCK_ON
enum{
    LPWG_READ = 1,
    LPWG_ENABLE,
    LPWG_LCD_X,
    LPWG_LCD_Y,
    LPWG_ACTIVE_AREA_X1,
    LPWG_ACTIVE_AREA_X2,
    LPWG_ACTIVE_AREA_Y1,
    LPWG_ACTIVE_AREA_Y2,
    LPWG_TAP_COUNT,
    LPWG_REPLY,
};
#endif

enum{
	DEBUG_NONE				= 0,
	DEBUG_BASE_INFO			= (1U << 0),	/* 1 */
	DEBUG_TRACE				= (1U << 1),	/* 2 */
	DEBUG_GET_DATA			= (1U << 2),	/* 4 */
	DEBUG_ABS				= (1U << 3),	/* 8 */
	DEBUG_BUTTON			= (1U << 4),	/* 16 */
	DEBUG_FW_UPGRADE		= (1U << 5),	/* 32 */
	DEBUG_GHOST				= (1U << 6),	/* 64 */
	DEBUG_IRQ_HANDLE		= (1U << 7),	/* 128 */
	DEBUG_POWER				= (1U << 8),	/* 256 */
	DEBUG_JITTER			= (1U << 9),	/* 512 */
	DEBUG_ACCURACY			= (1U << 10),	/* 1024 */
	DEBUG_NOISE             = (1U << 11),   /* 2048 */
};

#ifdef LGE_TOUCH_TIME_DEBUG
enum{
	TIME_ISR_START = 0,
	TIME_INT_INTERVAL,
	TIME_THREAD_ISR_START,
	TIME_WORKQUEUE_START,
	TIME_WORKQUEUE_END,
	TIME_FW_UPGRADE_START,
	TIME_FW_UPGRADE_END,
	TIME_PROFILE_MAX,
};

enum{
	DEBUG_TIME_PROFILE_NONE			= 0,
	DEBUG_TIME_INT_INTERVAL			= (1U << 0),	/* 1 */
	DEBUG_TIME_INT_IRQ_DELAY		= (1U << 1),	/* 2 */
	DEBUG_TIME_INT_THREAD_IRQ_DELAY	= (1U << 2),	/* 4 */
	DEBUG_TIME_DATA_HANDLE			= (1U << 3),	/* 8 */
	DEBUG_TIME_FW_UPGRADE			= (1U << 4),	/* 16 */
	DEBUG_TIME_PROFILE_ALL			= (1U << 5),	/* 32 */
};
#endif

enum{
	WORK_POST_COMPLATE = 0,
	WORK_POST_OUT,
	WORK_POST_ERR_RETRY,
	WORK_POST_ERR_CIRTICAL,
	WORK_POST_MAX,
#if defined(CONFIG_LGE_VU3_TOUCHSCREEN)
	WORK_POST_OUT_IGNORE_INT,
#endif
};

#ifdef CUST_G2_TOUCH
enum{
	IGNORE_INTERRUPT	= 100,
	NEED_TO_OUT,
	NEED_TO_INIT,
};

enum{
	TIME_EX_INIT_TIME,
	TIME_EX_FIRST_INT_TIME,
	TIME_EX_PREV_PRESS_TIME,
	TIME_EX_CURR_PRESS_TIME,
	TIME_EX_BUTTON_PRESS_START_TIME,
	TIME_EX_BUTTON_PRESS_END_TIME,
	TIME_EX_FIRST_GHOST_DETECT_TIME,
	TIME_EX_SECOND_GHOST_DETECT_TIME,
	TIME_EX_CURR_INT_TIME,
	TIME_EX_GHOST_EDGE_ZONE_START_TIME,
	TIME_EX_GHOST_EDGE_ZONE_END_TIME,
	TIME_EX_PRESSURE_MAX_START_TIME,
	TIME_EX_PROFILE_MAX
};
#endif

#define LGE_TOUCH_NAME		"lge_touch"

/* Debug Mask setting */
/* #define TOUCH_DEBUG_PRINT   (1) */
#define TOUCH_ERROR_PRINT   (1)
/* #define TOUCH_INFO_PRINT	(1) */

#if defined(TOUCH_INFO_PRINT)
#define TOUCH_INFO_MSG(fmt, args...) \
		do { \
			printk(KERN_INFO "[Touch] " fmt, ##args); \
		} while (0)
#else
#define TOUCH_INFO_MSG(fmt, args...) \
		do {} while (0)
#endif

#if defined(TOUCH_ERROR_PRINT)
#define TOUCH_ERR_MSG(fmt, args...) \
		do { \
			printk(KERN_ERR "[Touch E] [%s %d] " \
				fmt, __func__, __LINE__, ##args); \
		} while (0)
#else
#define TOUCH_ERR_MSG(fmt, args...) \
		do {} while (0)
#endif

#if defined(TOUCH_DEBUG_PRINT)
#define TOUCH_DEBUG_MSG(fmt, args...) \
		do { \
			printk(KERN_INFO "[Touch D] [%s %d] " \
					fmt, __func__, __LINE__, ##args); \
		} while (0)
#else
#define TOUCH_DEBUG_MSG(fmt, args...) \
		do {} while(0)
#endif

#if defined(CONFIG_MACH_MSM8974_G2_VZW) || defined(CONFIG_MACH_MSM8974_G2_TMO_US) || defined(CONFIG_MACH_MSM8974_G2_ATT)
#define ISIS_L2 /* block the exposure of privacy information */
#endif

int  touch_driver_register(struct touch_device_driver *driver);
void touch_driver_unregister(void);

void set_touch_handle(struct i2c_client *client, void *h_touch);
void *get_touch_handle(struct i2c_client *client);

#ifdef CONFIG_LGE_SECURITY_KNOCK_ON
void send_uevent(char* string[2]);
#endif
#ifdef I2C_SUSPEND_WORKAROUND
	extern bool atmel_touch_i2c_suspended;
#endif
int touch_i2c_read(struct i2c_client *client, u8 reg, int len, u8 *buf);
int touch_i2c_write(struct i2c_client *client, u8 reg, int len, u8 *buf);
int touch_i2c_write_byte(struct i2c_client *client, u8 reg, u8 data);

extern u32 touch_debug_mask;
extern u32 touch_time_debug_mask;

#endif
