#ifndef __BROADCAST_TCC3530_H__
#define __BROADCAST_TCC3530_H__

struct i2c_client*	FCI_GET_I2C_DRIVER(void);

int fc8300_power_on(void);
int fc8300_power_off(void);
int fc8300_is_power_on(void);
int fc8300_select_antenna(unsigned int sel);

#endif /*__BROADCAST_TCC3530_H__*/
