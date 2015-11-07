/*
 * vibration.c
 *
 *  Created on: 2015Äê7ÔÂ1ÈÕ
 *      Author: jk
 */

#include <stdlib.h>
#include <eat_interface.h>
#include <eat_periphery.h>

#include "vibration.h"
#include "thread.h"
#include "log.h"
#include "timer.h"
#include "thread_msg.h"
#include "setting.h"

#define VIBRATION_TRESHOLD 100000000

static eat_bool vibration_sendAlarm(void);
static void vibration_timer_handler(void);

static void mma_init(void);
static void mma_open(void);
static int mma_read(const int readAddress);
static int mma_write(const int writeAddress, int writeValue);
static void mma_Standby(void);
static void mma_Active(void);

static eat_bool mma_WhoAmI(void);
static void mma_ChangeDynamicRange(MMA_FULL_SCALE_EN mode);


void app_vibration_thread(void *data)
{
	EatEvent_st event;
	s32 ret;
	u8 write_buffer[10] = {0};

	LOG_INFO("vibration thread start.");

    /*
	ret = eat_i2c_open(EAT_I2C_OWNER_0, 0x1D, 100);
	if(EAT_DEV_STATUS_OK != ret)
	{
	    LOG_ERROR("vibration eat_i2c_open fail :ret=%d.", ret);
        return;
	}
	LOG_INFO("vibration eat_i2c_open success.");

    write_buffer[0] = MMA8X5X_CTRL_REG4;
	write_buffer[1] = 0x20;
    ret = eat_i2c_write(EAT_I2C_OWNER_0, write_buffer, 2);
    if(EAT_DEV_STATUS_OK != ret)
	{
		LOG_ERROR("vibration start MMA8X5X_CTRL_REG4 fail :ret=%d.", ret);
        return;
	}
	LOG_INFO("vibration start MMA8X5X_CTRL_REG4 success.");

    write_buffer[0] = MMA8X5X_CTRL_REG5;
	write_buffer[1] = 0x20;
	ret = eat_i2c_write(EAT_I2C_OWNER_0, write_buffer, 2);
    if(EAT_DEV_STATUS_OK != ret)
	{
		LOG_ERROR("vibration start MMA8X5X_CTRL_REG5 fail :ret=%d.", ret);
        return;
	}
	LOG_INFO("vibration start MMA8X5X_CTRL_REG5 success.");

    write_buffer[0] = MMA8X5X_TRANSIENT_CFG;
	write_buffer[1] = 0x1e;
	ret = eat_i2c_write(EAT_I2C_OWNER_0, write_buffer, 2);
    if(EAT_DEV_STATUS_OK != ret)
	{
		LOG_ERROR("vibration start MMA8X5X_FF_MT_CFG fail :ret=%d.", ret);
        return;
	}
	LOG_INFO("vibration start MMA8X5X_FF_MT_CFG success.");

    write_buffer[0] = MMA8X5X_TRANSIENT_THS;
	write_buffer[1] = 0x1;
	ret = eat_i2c_write(EAT_I2C_OWNER_0, write_buffer, 2);
    if(EAT_DEV_STATUS_OK != ret)
	{
		LOG_ERROR("vibration start MMA8X5X_FF_MT_THS fail :ret=%d.", ret);
        return;
	}
	LOG_INFO("vibration start MMA8X5X_FF_MT_THS success.");

    write_buffer[0] = MMA8X5X_HP_FILTER_CUTOFF;
	write_buffer[1] = 0x3;
	ret = eat_i2c_write(EAT_I2C_OWNER_0, write_buffer, 2);
    if(EAT_DEV_STATUS_OK != ret)
	{
		LOG_ERROR("vibration start MMA8X5X_FF_MT_THS fail :ret=%d.", ret);
        return;
	}
	LOG_INFO("vibration start MMA8X5X_FF_MT_THS success.");

    write_buffer[0] =  MMA8X5X_TRANSIENT_COUNT;
	write_buffer[1] = 0x40;
	ret = eat_i2c_write(EAT_I2C_OWNER_0, write_buffer, 2);
    if(EAT_DEV_STATUS_OK != ret)
	{
		LOG_ERROR("vibration start MMA8X5X_FF_MT_COUNT fail :ret=%d.", ret);
        return;
	}
	LOG_INFO("vibration start MMA8X5X_FF_MT_COUNT success.");

    write_buffer[0] = MMA8X5X_CTRL_REG1;
	write_buffer[1] = 0x01;
	ret = eat_i2c_write(EAT_I2C_OWNER_0, write_buffer, 2);
	if(EAT_DEV_STATUS_OK != ret)
	{
		LOG_ERROR("vibration start MMA8X5X_CTRL_REG1 fail :ret=%d.", ret);
        return;
	}
	LOG_INFO("vibration start MMA8X5X_CTRL_REG1 success.");*/

    mma_init();

	eat_timer_start(TIMER_VIBRATION, setting.vibration_timer_period);
	while(EAT_TRUE)
	{
        eat_get_event_for_user(THREAD_VIBRATION, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER:
				switch (event.data.timer.timer_id)
				{
    				case TIMER_VIBRATION:
    					vibration_timer_handler();
    					eat_timer_start(TIMER_VIBRATION, setting.vibration_timer_period);
    					break;

    				default:
    					LOG_ERROR("timer(%d) expire!", event.data.timer.timer_id);
    					break;
				}
				break;

            default:
            	LOG_ERROR("event(%d) not processed!", event.event);
                break;
        }
    }
}

#if 1
static void mma_init(void)
{
    mma_open();
    mma_WhoAmI();

    mma_Standby();
    //LOG_DEBUG("MMA8X5X_SYSMOD = %2x", mma_read(MMA8X5X_SYSMOD));

    mma_write(MMA8X5X_CTRL_REG4, 0x20);
    mma_write(MMA8X5X_CTRL_REG5, 0x20);
    mma_write(MMA8X5X_TRANSIENT_CFG, 0x1e);
    mma_write(MMA8X5X_TRANSIENT_THS, 0x01);
    mma_write(MMA8X5X_HP_FILTER_CUTOFF, 0x03);
    mma_write(MMA8X5X_TRANSIENT_COUNT, 0x40);

    mma_Active();
    //LOG_DEBUG("MMA8X5X_SYSMOD = %2x", mma_read(MMA8X5X_SYSMOD));
}

static void mma_open(void)
{
    s32 ret;

    ret = eat_i2c_open(EAT_I2C_OWNER_0, 0x1D, 100);
	if(EAT_DEV_STATUS_OK != ret)
	{
	    LOG_ERROR("mma open i2c fail :ret=%d.", ret);
	}
	LOG_INFO("mma open i2c success.");

    return;
}

static int mma_read(const int readAddress)
{
    unsigned char readAddressBuff[1];
    unsigned char readBuff[1];

    readAddressBuff[0] = readAddress;
    eat_i2c_read(EAT_I2C_OWNER_0, readAddressBuff, 1, readBuff, 1);

    return readBuff[0];
}

static int mma_write(const int writeAddress, int writeValue)
{
    unsigned char write[2] = {0};

    write[0] = writeAddress;
    write[1] = writeValue;

    return eat_i2c_write(EAT_I2C_OWNER_0, write, 2);
}

static void mma_Standby(void)
{
    mma_write(MMA8X5X_CTRL_REG1, (mma_read(MMA8X5X_CTRL_REG1) & ~0x01));

    return;
}

static void mma_Active(void)
{
    mma_write(MMA8X5X_CTRL_REG1, (mma_read(MMA8X5X_CTRL_REG1) | 0x01));

    return;
}

static eat_bool mma_WhoAmI(void)
{
    //Ox4a
    if(0x4a == mma_read(MMA8X5X_WHO_AM_I))
    {
        LOG_DEBUG("mma get 0x4a.");
        return EAT_TRUE;
    }
    else
    {
        LOG_ERROR("mma no 0x4a.");
        return EAT_FALSE;
    }
}

/* Full-scale selection
 * 00-2g
 * 01-4g
 * 10-8g
 * 11-reserved
 */
static void mma_ChangeDynamicRange(MMA_FULL_SCALE_EN mode)
{
    mma_Standby();

    switch(mode)
    {
        case MMA_FULL_SCALE_2G:
            LOG_DEBUG("change dynamic range as 2g.");
            mma_write(MMA8X5X_XYZ_DATA_CFG, (mma_read(MMA8X5X_XYZ_DATA_CFG) & ~0x03));
            break;

        case MMA_FULL_SCALE_4G:
            LOG_DEBUG("change dynamic range as 4g.");
            mma_write(MMA8X5X_XYZ_DATA_CFG, (mma_read(MMA8X5X_XYZ_DATA_CFG) & ~0x02));
            mma_write(MMA8X5X_XYZ_DATA_CFG, (mma_read(MMA8X5X_XYZ_DATA_CFG) | 0x01));
            break;

        case MMA_FULL_SCALE_8G:
            LOG_DEBUG("change dynamic range as 8g.");
            mma_write(MMA8X5X_XYZ_DATA_CFG, (mma_read(MMA8X5X_XYZ_DATA_CFG) & ~0x01));
            mma_write(MMA8X5X_XYZ_DATA_CFG, (mma_read(MMA8X5X_XYZ_DATA_CFG) | 0x02));
            break;

        default:
            LOG_ERROR("unknown MMA_FULL_SCALE_EN!");
            break;
    }

    mma_Active();
}

#endif

static void vibration_timer_handler(void)
{
    u8 read_buffer[MMA8X5X_BUF_SIZE] = { 0 };
    u8 write_buffer[MMA8X5X_BUF_SIZE] = { 0 };
    s32 ret;
    long delta;

    if(EAT_TRUE == vibration_fixed())
    {
        //LOG_INFO("vibration is already fixed.");
        //write_buffer[0] = MMA8X5X_TRANSIENT_CFG;
        //ret = eat_i2c_read(EAT_I2C_OWNER_0, write_buffer, 1, read_buffer, 4);
        //LOG_DEBUG("MMA8X5X_FF_MT_CFG=%x, %x, %x, %x", read_buffer[0], read_buffer[1], read_buffer[2], read_buffer[3]);

        if (ret != 0)
        {
    	    LOG_ERROR("i2c test eat_i2c_read 0AH fail :ret=%d", ret);
            return;
        }
        else
        {
            if(mma_read(MMA8X5X_TRANSIENT_SRC) & 0x40)
            {
                vibration_sendAlarm();
            }
        }
    }
    else
    {
        //LOG_INFO("vibration is not fixed.");
        return;
    }
}

static eat_bool vibration_sendAlarm(void)
{
    u8 msgLen = sizeof(MSG_THREAD) + 1;
    MSG_THREAD* msg = allocMsg(msgLen);
    unsigned char* alarmType = (unsigned char*)msg->data;

    msg->cmd = CMD_THREAD_VIBRATE;
    msg->length = 1;
    *alarmType = ALARM_VIBRATE;

    LOG_DEBUG("vibration alarm:cmd(%d),length(%d),data(%d)", msg->cmd, msg->length, *(unsigned char*)msg->data);
    return sendMsg(THREAD_VIBRATION, THREAD_MAIN, msg, msgLen);
}

