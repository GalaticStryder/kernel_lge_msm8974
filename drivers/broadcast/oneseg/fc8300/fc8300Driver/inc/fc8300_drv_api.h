
#ifndef __FC8300_API_H__
#define __FC8300_API_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int lock;
	int ber;
	int per;
	int ErrTSP;
	int TotalTSP;
	int antenna_level;

	int tmccinfo;
	int receive_status;
	int rssi;
	int scan_status;
	int sysinfo;
	int cn;

	int ber_a;
	int per_a;
	int layerinfo_a;
	int total_tsp_a;

	int ber_b;
	int per_b;
	int layerinfo_b;
	int total_tsp_b;

	int ber_c;
	int per_c;
	int layerinfo_c;
	int total_tsp_c;

	int fullseg_oneseg_flag;
	int antenna_level_fullseg;
	int antenna_level_oneseg;
	int agc;
	int ber_1seg;
	int per_1seg;
	int total_tsp_1seg;
	int err_tsp_1seg;
	int ber_fullseg;
	int per_fullseg;
	int total_tsp_fullseg;
	int err_tsp_fullseg;
} fc8300Status_t;
void tunerbb_drv_hw_setting(void);
void tunerbb_drv_hw_init(void);
void tunerbb_drv_hw_deinit(void);

int tunerbb_drv_fc8300_init(int mode);
int tunerbb_drv_fc8300_stop(void);
int tunerbb_drv_fc8300_set_channel(s32 f_rf, u16 mode, u8 subch);
int tunerbb_drv_fc8300_Get_SyncStatus(void);
int tunerbb_drv_fc8300_Get_SignalInfo(fc8300Status_t *st, s32 brd_type);

#ifdef __cplusplus
};
#endif

#endif
