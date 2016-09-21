/* MMA8652 C library - small 3-axis digital accelerometer from Freescale
 * 
 * This file contains the hardware-independent functions
 *
 * 2014 - Tom Magnier : tom@tmagnier.fr
 */
 
#include "mma8652.h"
#include "eat_type.h"

bool mma8652_init(void)
{
	mma8652_i2c_init();

	if(mma8652_verify_device_id())
	{
        unsigned char readbuf[3];
        bool ret = mma8652_i2c_register_read(MMA8652_REG_OUT_X_MSB, readbuf, 3);
        if(readbuf[0] != 0 || readbuf[1] != 0 || readbuf[2] != 0)
        {
            return EAT_TRUE;
        }
	}
    return EAT_FALSE;
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
	
	if(!mma8652_i2c_register_read(MMA8652_REG_CTRL_REG1, &value, sizeof(value)))
		return false;
	
	return mma8652_i2c_register_write(MMA8652_REG_CTRL_REG1, 
		value & ~(MMA8652_CTRL_REG1_ACTIVE));
}

bool mma8652_active(void)
{
	//Read current value of CTRL_REG1 register
	uint8_t value = 0;
	
	if(!mma8652_i2c_register_read(MMA8652_REG_CTRL_REG1, &value, sizeof(value)))
		return false;
	
	return mma8652_i2c_register_write(MMA8652_REG_CTRL_REG1, 
		value | MMA8652_CTRL_REG1_ACTIVE);
}

//TODO: return bool instead of void
void mma8652_config(void)
{
    uint8_t value = 0;

    mma8652_standby();

    mma8652_i2c_register_write(MMA8652_REG_CTRL_REG4, MMA8652_CTRL_REG4_INT_EN_TRANS);
    mma8652_i2c_register_write(MMA8652_REG_CTRL_REG5, MMA8652_CTRL_REG5_INT_CFG_TRANS);
    mma8652_i2c_register_write(MMA8652_REG_TRANSIENT_CFG, MMA8652_TRANSIENT_CFG_XTEFE | MMA8652_TRANSIENT_CFG_YTEFE | MMA8652_TRANSIENT_CFG_ZTEFE | MMA8652_TRANSIENT_CFG_ELE);
    mma8652_i2c_register_write(MMA8652_REG_TRANSIENT_THS, 0x01 & MMA8652_TRANSIENT_THS_MSK);
    mma8652_i2c_register_write(MMA8652_REG_HP_FILTER_CUTOFF, MMA8652_HP_FILTER_SEL_MSK);
    mma8652_i2c_register_write(MMA8652_REG_TRANSIENT_COUNT, 0x40);

    mma8652_i2c_register_read(MMA8652_REG_CTRL_REG1, &value, sizeof(value));

    mma8652_i2c_register_write(MMA8652_REG_CTRL_REG1, value | MMA8652_CTRL_REG1_F_READ);

    mma8652_active();
}
