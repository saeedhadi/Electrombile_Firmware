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

#define VIBRATION_TRESHOLD 100000000

static u16 avoid_freq_count;
static eat_bool avoid_freq_flag;


void app_vibration_thread(void *data)
{
	EatEvent_st event;

	LOG_INFO("vibration thread start.");

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

static void vibration_timer_handler(void)
{
    static eat_bool isFirstTime = EAT_TRUE;
    static eat_bool isMoved = EAT_TRUE;
    static int timerCount = 0;

    #if 0
    set_vibration_state(EAT_TRUE);//TO DO
    #endif

    if(++avoid_freq_count == 30)
    {
        avoid_freq_count = 0;
        avoid_freq_flag = EAT_FALSE;
    }
    if(mma_read(MMA8X5X_TRANSIENT_SRC) & 0x40)
    {
        /* At the first time, the value of MMA8X5X_TRANSIENT_SRC is strangely 0x60.
         * Do not send alarm at the first time.
         */
        if(isFirstTime)
        {
            isFirstTime = EAT_FALSE;

            isMoved = EAT_FALSE;
        }
        else
        {
            isMoved = EAT_TRUE;
        }
    }
    else
    {
        isMoved = EAT_FALSE;
    }

    if(EAT_TRUE == vibration_fixed())
    {
        timerCount = 0;

        if(isMoved && avoid_freq_flag == EAT_FALSE)
        {
            avoid_freq_flag = EAT_TRUE;
            avoid_freq_count = 0;
            vibration_sendAlarm();
        }
    }
    else
    {
        if(get_autodefend_state())
        {
            if(isMoved)
            {
                timerCount = 0;
            }
            else
            {
                timerCount++;

                if(timerCount * setting.vibration_timer_period >= (get_autodefend_period() * 60000))
                {
                    LOG_INFO("vibration state auto locked.");
                    set_vibration_state(EAT_TRUE);
                }
            }
        }
    }

    return;
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


static void mma_init(void)
{
    mma_open();
    //mma_WhoAmI();

    mma_Standby();
    //LOG_DEBUG("MMA8X5X_TRANSIENT_SRC = %02x", mma_read(MMA8X5X_TRANSIENT_SRC));

    mma_write(MMA8X5X_CTRL_REG4, 0x20);
    mma_write(MMA8X5X_CTRL_REG5, 0x20);
    mma_write(MMA8X5X_TRANSIENT_CFG, 0x1e);
    mma_write(MMA8X5X_TRANSIENT_THS, 0x01);
    mma_write(MMA8X5X_HP_FILTER_CUTOFF, 0x03);
    mma_write(MMA8X5X_TRANSIENT_COUNT, 0x40);

    mma_Active();
    //LOG_DEBUG("MMA8X5X_TRANSIENT_SRC = %02x", mma_read(MMA8X5X_TRANSIENT_SRC));
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

