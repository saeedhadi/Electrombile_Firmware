/* MMA8652 C library - small 3-axis digital accelerometer from Freescale
 * 
 * This file contains the hardware-independent functions
 *
 * 2014 - Tom Magnier : tom@tmagnier.fr
 */
 
#include "mma8652.h"

bool mma8652_init(void)
{
	mma8652_i2c_init();
	
	return mma8652_verify_device_id();
}

bool mma8652_verify_device_id(void)
{
	uint8_t device_id = 0;
	
	//Read device ID from WHO_AM_I register
	mma8652_i2c_register_read(MMA8652_REG_WHO_AM_I, &device_id, 1);
	
	return (device_id == MMA8652_DEVICE_ID);
}

bool mma8652_standby(void)
{
	//Read current value of CTRL_REG1 register
	uint8_t value = 0;
	
	if(!mma8652_i2c_register_read(MMA8652_REG_CTRL_REG1, &value, 1))
		return false;
	
	return mma8652_i2c_register_write(MMA8652_REG_CTRL_REG1, 
		value & ~(MMA8652_CTRL_REG1_ACTIVE));
}

bool mma8652_active(void)
{
	//Read current value of CTRL_REG1 register
	uint8_t value = 0;
	
	if(!mma8652_i2c_register_read(MMA8652_REG_CTRL_REG1, &value, 1))
		return false;
	
	return mma8652_i2c_register_write(MMA8652_REG_CTRL_REG1, 
		value | MMA8652_CTRL_REG1_ACTIVE);
}
