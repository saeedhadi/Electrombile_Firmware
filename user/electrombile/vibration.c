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

	write_buffer[0] = MMA8X5X_CTRL_REG1;
	write_buffer[1] = 0x01;
	ret = eat_i2c_write(EAT_I2C_OWNER_0, write_buffer, 2);
	if(EAT_DEV_STATUS_OK != ret)
	{
		LOG_ERROR("vibration start sample fail :ret=%d.", ret);
        return;
	}
	LOG_INFO("vibration start sample success.");

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

        write_buffer[0] = MMA8X5X_OUT_X_MSB;
        ret = eat_i2c_read(EAT_I2C_OWNER_0, &write_buffer[0], 1, read_buffer, MMA8X5X_BUF_SIZE);
        if (ret != 0)
        {
    		LOG_ERROR("i2c test eat_i2c_read 0AH fail :ret=%d", ret);
            return;
        }
        else
        {
            //LOG_DEBUG("read_length=%d", sizeof(read_buffer));

            datax[vibration_data_i] = ((read_buffer[0] << 8) & 0xff00) | read_buffer[1];
            datay[vibration_data_i] = ((read_buffer[2] << 8) & 0xff00) | read_buffer[3];
            dataz[vibration_data_i] = ((read_buffer[4] << 8) & 0xff00) | read_buffer[5];
            data_x2y2z2[vibration_data_i] = datax[vibration_data_i] * datax[vibration_data_i]
                                          + datay[vibration_data_i] * datay[vibration_data_i]
                                          + dataz[vibration_data_i] * dataz[vibration_data_i];
            delta = abs(data_x2y2z2[vibration_data_i] - 282305280);

            //LOG_DEBUG("\rdatax=%d, datay=%d, dataz=%d, ", datax[vibration_data_i], datay[vibration_data_i], dataz[vibration_data_i]);
            //LOG_DEBUG("\rdata_x2y2z2=%d, delta=%d", data_x2y2z2[vibration_data_i], delta);

            if (delta > VIBRATION_TRESHOLD)
            {
                vibration_sendAlarm();
            }

            vibration_data_i++;
            if (vibration_data_i == 10)
                vibration_data_i = 0;
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
