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

static short datax[10],datay[10],dataz[10];
static long data_x2y2z2[10];
static int vibration_data_i=0;
#define VIBRATION_TRESHOLD 100000000

static eat_bool vibration_sendMsg2Main(MSG_THREAD* msg, u8 len);
static eat_bool vibration_sendAlarm(void);
static void vibration_timer_handler(void);

float fangcha(long *array,int len)
{
   int i;
   float sum=0;
   float ave;
   for(i=0;i<len;i++)
      sum+=array[i];
   ave=sum/len;
   sum=0;
   for(i=0;i<len;i++)
      sum+=(array[i]-ave)*(array[i]-ave);
   ave=sum/len;
   return ave;
}

void app_vibration_thread(void *data)
{
	EatEvent_st event;
	s32 ret;
	u8 write_buffer[10] = {0};

	LOG_INFO("vibration thread start.");

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
       write_buffer[0] =  MMA8X5X_TRANSIENT_CFG;
	write_buffer[1] = 0x1e;
	ret = eat_i2c_write(EAT_I2C_OWNER_0, write_buffer, 2);
       if(EAT_DEV_STATUS_OK != ret)
	{
		LOG_ERROR("vibration start MMA8X5X_FF_MT_CFG fail :ret=%d.", ret);
              return;
	}
	LOG_INFO("vibration start MMA8X5X_FF_MT_CFG success.");
       write_buffer[0] =  MMA8X5X_TRANSIENT_THS;
	write_buffer[1] = 0x1;   
	ret = eat_i2c_write(EAT_I2C_OWNER_0, write_buffer, 2);
       if(EAT_DEV_STATUS_OK != ret)
	{
		LOG_ERROR("vibration start MMA8X5X_FF_MT_THS fail :ret=%d.", ret);
              return;
	}
	LOG_INFO("vibration start MMA8X5X_FF_MT_THS success.");
    
       write_buffer[0] =  MMA8X5X_HP_FILTER_CUTOFF;
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
	LOG_INFO("vibration start MMA8X5X_CTRL_REG1 success.");

  

	eat_timer_start(TIMER_VIBRATION, setting.vibration_timer_period);

	while(EAT_TRUE)
	{
        eat_get_event_for_user(THREAD_VIBRATION, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER :
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
    u8 read_buffer[MMA8X5X_BUF_SIZE] = { 0 };
    u8 write_buffer[MMA8X5X_BUF_SIZE] = { 0 };
    s32 ret;
    long delta;

    if(EAT_TRUE == vibration_fixed())
    {
        //LOG_INFO("vibration is already fixed.");
        write_buffer[0] = MMA8X5X_TRANSIENT_CFG;
        ret = eat_i2c_read(EAT_I2C_OWNER_0, &write_buffer[0], 1, read_buffer, 4);  
        LOG_DEBUG("MMA8X5X_FF_MT_CFG=%x, %x, %x, %x", read_buffer[0], read_buffer[1], read_buffer[2], read_buffer[3]);
       
        if (ret != 0)
        {
    	     LOG_ERROR("i2c test eat_i2c_read 0AH fail :ret=%d", ret);
            return;
        }
        else
        {
            if(read_buffer[1]&0x40)
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

static eat_bool vibration_sendMsg2Main(MSG_THREAD* msg, u8 len)
{
    return sendMsg(THREAD_VIBRATION, THREAD_MAIN, msg, len);
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
    return vibration_sendMsg2Main(msg, msgLen);
}
