/*
 * vibration.c
 *
 *  Created on: 2015ï¿½ï¿½7ï¿½ï¿½1ï¿½ï¿½
 *      Author: jk
 */

#include <stdlib.h>
#include <eat_interface.h>
#include <eat_periphery.h>

#include "vibration.h"
#include "thread.h"
#include "log.h"
#include "timer.h"
#include "data.h"
#include "thread_msg.h"
#include "setting.h"
#include "client.h"


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
#define MAX_MOVE_DATA_LEN   500
#define MOVE_TIMER_PERIOD    10
#define MOVE_TRESHOLD   50;


static u16 avoid_freq_count;
static eat_bool avoid_freq_flag;
eat_bool isMoved = EAT_FALSE;//ÒÆ¶¯±êÖ¾
void DigitalIntegrate(float * sour, float * dest,int len,float cycle)
{
	int i;
	if (len==0)
		return ;
	dest[0]=0;
	for (i=1;i<len;i++)
	{
		dest[i]=dest[i-1]+(sour[i-1]+sour[i])*cycle/2;
	}
}

static void move_alarm_timer_handler()
{
    char addbuf[2];
    char readbuf[3];
    int i;
    float tmp[3]={0};
    short temp;
    static float x_data[MAX_MOVE_DATA_LEN], y_data[MAX_MOVE_DATA_LEN], z_data[MAX_MOVE_DATA_LEN];
    float temp_data[MAX_MOVE_DATA_LEN];
    static int timerCount = 0;
    addbuf[0] = MMA8X5X_OUT_X_MSB;
    eat_i2c_read(EAT_I2C_OWNER_0, addbuf, 1, readbuf, 3);
    temp = readbuf[0]<<8;
    x_data[timerCount] = temp/256;
     temp = readbuf[1]<<8;
    y_data[timerCount] = temp/256;
     temp = readbuf[2]<<8;
    z_data[timerCount] = temp/256;
    timerCount++;

    if(timerCount<MAX_MOVE_DATA_LEN)
    {
        eat_timer_start(TIMER_MOVE_ALARM, MOVE_TIMER_PERIOD);
    }
    else
    {
        timerCount = 0;
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            tmp[0] += (x_data[i]/MAX_MOVE_DATA_LEN);
            tmp[1] += (y_data[i]/MAX_MOVE_DATA_LEN);
            tmp[2] += (z_data[i]/MAX_MOVE_DATA_LEN);
        }
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            x_data[i] = x_data[i] - tmp[0];
            y_data[i] = y_data[i] - tmp[1];
            z_data[i] = z_data[i] - tmp[2];
        }
        DigitalIntegrate(x_data, temp_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        DigitalIntegrate(temp_data, x_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            if(x_data[0]<abs(x_data[i]))
            {
                x_data[0] = x_data[i];
            }
            if(x_data[i]>1||x_data[i]<-1)
            {
                vibration_sendAlarm();
                LOG_DEBUG("MOVE_TRESHOLD_X[%d]   = %f", i,x_data[i]);
               return;
            }

        }
        LOG_DEBUG("MAX_X  = %f", x_data[0]);
        DigitalIntegrate(y_data, temp_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);

        DigitalIntegrate(temp_data, y_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            if(y_data[i]>1||y_data[i]<-1)
            {
                vibration_sendAlarm();
                LOG_DEBUG("MOVE_TRESHOLD_Y[%d]   = %f",i, y_data[i]);

                return;
            }
            if(y_data[0]<abs(y_data[i]))
            {
                y_data[0] = y_data[i];
            }

        }
         LOG_DEBUG("MAX_Y  = %f", y_data[0]);
        DigitalIntegrate(z_data, temp_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        DigitalIntegrate(temp_data, z_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            if(z_data[i]>1||z_data[i]<-1)
            {
                vibration_sendAlarm();
                LOG_DEBUG("MOVE_TRESHOLD_Z[%d]   = %f",i, z_data[i]);
                return;
            }
            if(z_data[0]<abs(z_data[i]))
            {
                z_data[0] = z_data[i];
            }
        }
        LOG_DEBUG("MAX_z  = %f", z_data[0]);
    }


}
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
                    case TIMER_MOVE_ALARM:
                        move_alarm_timer_handler();

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

    static int timerCount = 0;

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

            avoid_freq_count = 0;
            eat_timer_start(TIMER_MOVE_ALARM, MOVE_TIMER_PERIOD);
            //vibration_sendAlarm();  //bec use displacement judgement , there do not alarm
        }
    }
    else
    {
        if(get_autodefend_state())
        {
            if(isMoved)
            {
                timerCount = 0;
                LOG_INFO("timerCount = 0 now !");
            }
            else
            {
                timerCount++;

                if(timerCount * setting.vibration_timer_period >= (get_autodefend_period() * 60000))
                {
                    LOG_INFO("vibration state auto locked.");
                    mileagehandle(EAT_FALSE, EAT_TRUE);
                    send_autodefendstate_msg(EAT_FALSE);
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
    avoid_freq_flag = EAT_TRUE;
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
    mma_write(MMA8X5X_CTRL_REG1, (mma_read(MMA8X5X_CTRL_REG1) | 0x02));
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

