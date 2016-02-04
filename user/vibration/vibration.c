/*
 * vibration.c
 *
 *  Created on: 2015��7��1��
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
#include "tool.h"
#include "thread_msg.h"
#include "setting.h"
#include "mma8652.h"

#define READ_BUFF_SIZE 2048

static eat_bool vibration_sendAlarm(void);
static void vibration_timer_handler(void);
static void avoid_fre_send(eat_bool state);


#define MAX_MOVE_DATA_LEN   500
#define MOVE_TIMER_PERIOD    10

#define MOVE_THRESHOLD 5

static eat_bool avoid_freq_flag = EAT_FALSE;
eat_bool isMoved = EAT_FALSE;

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
    unsigned char readbuf[3];
    int i;
    float tmp[3]={0};
    short temp;
    static float x_data[MAX_MOVE_DATA_LEN], y_data[MAX_MOVE_DATA_LEN], z_data[MAX_MOVE_DATA_LEN];
    float temp_data[MAX_MOVE_DATA_LEN];
    static int timerCount = 0;
    bool ret = mma8652_i2c_register_read(MMA8652_REG_OUT_X_MSB, readbuf, 3);
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
                x_data[0] = abs(x_data[i]);
            }
            if(x_data[i] > MOVE_THRESHOLD)
            {
                if(EAT_TRUE == vibration_fixed())
                {
                    vibration_sendAlarm();
                    LOG_DEBUG("MOVE_TRESHOLD_Z[%d]   = %f",i, x_data[i]);
                }

               return;
            }

        }
        LOG_DEBUG("MAX_X  = %f", x_data[0]);
        DigitalIntegrate(y_data, temp_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);

        DigitalIntegrate(temp_data, y_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            if(y_data[i] > MOVE_THRESHOLD)
            {
                if(EAT_TRUE == vibration_fixed())
                {
                    vibration_sendAlarm();
                    LOG_DEBUG("MOVE_TRESHOLD_Z[%d]   = %f",i, y_data[i]);
                }

                return;
            }
            if(y_data[0]<abs(y_data[i]))
            {
                y_data[0] = abs(y_data[i]);
            }

        }
        LOG_DEBUG("MAX_Y  = %f", y_data[0]);
        DigitalIntegrate(z_data, temp_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        DigitalIntegrate(temp_data, z_data, MAX_MOVE_DATA_LEN,MOVE_TIMER_PERIOD/1000.0);
        for(i=0;i<MAX_MOVE_DATA_LEN;i++)
        {
            if(z_data[i] > MOVE_THRESHOLD)
            {
                if(EAT_TRUE == vibration_fixed())
                {
                    vibration_sendAlarm();
                    LOG_DEBUG("MOVE_TRESHOLD_Z[%d]   = %f",i, z_data[i]);
                }
                return;
            }
            if(z_data[0]<abs(z_data[i]))
            {
                z_data[0] = abs(z_data[i]);
            }
        }
        LOG_DEBUG("MAX_z  = %f", z_data[0]);
    }


}
void BT_at_read_handler()
{
    unsigned char *buf_p1 = NULL;
    unsigned char *buf_p2 = NULL;
    unsigned char  buf[READ_BUFF_SIZE] = {0};  //���ڶ�ȡATָ�����Ӧ
    unsigned int len = 0;
    unsigned int count = 0, cellCount = 0;
    static double gpstimes = 0.0;
    int _mcc = 0;
    int _mnc = 0;
    int lac = 0;
    int cellid = 0;
    int rxl = 0;

    len = eat_modem_read(buf, READ_BUFF_SIZE);
    LOG_DEBUG("modem read, len=%d, buf=\r\n%s", len, buf);

    buf_p1 = tool_StrstrAndReturnEndPoint(buf, "AT+BTPOWER=1\r\r\n");
    if(NULL != buf_p1)
    {
        buf_p2 = (unsigned char*)strstr(buf_p1, "OK");
        if(buf_p1 == buf_p2)
        {
            LOG_DEBUG("turn on BT power OK.");
        }
    }
        /*
        +BTSCAN: 0,1,"hongmi",9c:99:a0:3b:67:b8,-58
        +BTSCAN: 1
        */
    buf_p1 = tool_StrstrAndReturnEndPoint(buf, "+BTSCAN: ");
    if(NULL != buf_p1)
    {
       buf_p2 = tool_StrstrAndReturnEndPoint(buf, "hongmi");//�ĳ�app�޸ĵ��ֻ�����
       if(NULL != buf_p2)
        {
            set_vibration_state(EAT_FALSE);
           LOG_DEBUG("set defend switch off.");
       }       
    }
}

void app_vibration_thread(void *data)
{
	EatEvent_st event;
	bool ret;
	static int number=0;
	LOG_INFO("vibration thread start.");
      
	ret = mma8652_init();
	if (!ret)
	{
        LOG_ERROR("mma8652 init failed");
	}
	else
	{
	    mma8652_config();
	}
    tool_modem_write("AT+BTPOWER=1\n");
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
                        
                       // if(get_ScanCompletion_state())
                       if(number++>30)
                        {       LOG_DEBUG("BTSCAN.");
                                 number = 0 ;
                                 tool_modem_write("AT+BTSCAN=1,10\n");
                        }
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
            case EAT_EVENT_MDM_READY_RD:
                LOG_DEBUG("BT AT read.");
                BT_at_read_handler();
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

    char transient_src = 0;

    avoid_fre_send(EAT_TRUE);

    mma8652_i2c_register_read(MMA8652_REG_TRANSIENT_SRC, &transient_src, sizeof(transient_src));
    if(transient_src & MMA8652_TRANSIENT_SRC_EA)
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

            avoid_fre_send(EAT_FALSE);
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

                    cmd_Autodefendstate(EAT_FALSE);

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
static void avoid_fre_send(eat_bool state)
{
    static u16 avoid_freq_count;

    if(state == EAT_TRUE)
    {
        if(++avoid_freq_count == 10)
        {
            avoid_freq_count = 0;
            avoid_freq_flag = EAT_FALSE;
        }
    }
    else
    {
        avoid_freq_count = 0;
    }
}


