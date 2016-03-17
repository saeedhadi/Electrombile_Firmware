
#include <eat_periphery.h>
#include <eat_timer.h>
#include <eat_interface.h>


#include <math.h>

#include "thread_msg.h"
#include "thread.h"
#include "battery.h"
#include "log.h"
#include "adc.h"

#define MAX_VLOTAGE_NUM 10

/*  Translate advalue to actual value   */
#define ADvalue_2_Realvalue(x) x*103/3/1000.f //unit mV, 3K & 100k divider

static u32 StorageVoltage;


/*
*fun:get dunp battery -x%
*/
static char battery_getbattery(void)
{
    char percent = 0;

    percent = (char)exp((ADvalue_2_Realvalue(StorageVoltage) -37.873)/2.7927);

    if(percent > 100)
    {
        percent =100;
    }

    return percent;
}

/*
*fun:save the MAX_VLOTAGE_NUM times voltage
*/
static void battery_save_voltage(u32 voltage[])
{
    int count = 0;
    u32 tmp_voltage = 0;

    for(count = 0;count < MAX_VLOTAGE_NUM;count++)
    {
        tmp_voltage += voltage[count];
    }
    StorageVoltage = tmp_voltage / MAX_VLOTAGE_NUM;
}

/*
*fun:event adc proc
*/
static void battery_event_adc(EatEvent_st *event)
{
    static int count;
    static u32 voltage[MAX_VLOTAGE_NUM];

    if(event->data.adc.pin == ADC_VOLTAGE)
    {
        if(count >= MAX_VLOTAGE_NUM)
        {
            count =0;
            battery_save_voltage(voltage);
        }

        voltage[count++] = event->data.adc.v;

    }
    else
    {
        LOG_ERROR("ADC_433 = %d",event->data.adc.v);
    }
}

/*
*fun:battery event handler
*/
static eat_bool battery_event_handler(void)
{
    eat_bool ret;
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(BATTERY_INFO);
    MSG_THREAD* msg = allocMsg(msgLen);
    BATTERY_INFO* msg_state = 0;

    if (!msg)
    {
        LOG_ERROR("alloc msg failed!");
        return EAT_FALSE;
    }

    msg->cmd = CMD_THREAD_BATTERY;
    msg->length = sizeof(BATTERY_INFO);

    msg_state = (BATTERY_INFO*)msg->data;
    msg_state->percent = battery_getbattery();
    msg_state->miles = 0;

    LOG_DEBUG("send battery info msg to main thread!");
    ret = sendMsg(THREAD_MAIN, msg, msgLen);

    return ret;

}


void app_battery_thread(void *data)
{
	EatEvent_st event;
    MSG_THREAD* msg = 0;
    int rc;

	LOG_INFO("battery thread start.");

    while(!eat_adc_get(ADC_VOLTAGE, ADC_VOLTAGE_PERIOD, NULL))
    {
        eat_sleep(100);//if failed ,try again after 100ms
    }

	while(EAT_TRUE)
	{
        eat_get_event_for_user(THREAD_BATTERY, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER:
                switch (event.data.timer.timer_id)
                {
                    default:
                        LOG_ERROR("timer(%d) expire!", event.data.timer.timer_id);
                        break;
                }

            case EAT_EVENT_ADC:

                battery_event_adc(&event);

                break;

            case EAT_EVENT_USER_MSG:
                msg = (MSG_THREAD*) event.data.user_msg.data_p;

                switch (msg->cmd)
                {
                    case CMD_THREAD_BATTERY:
                        battery_event_handler();
                        break;

                    default:
                        LOG_ERROR("cmd(%d) not processed!", msg->cmd);
                        break;
                }
                break;

            default:
            	LOG_ERROR("event(%d) not processed!", event.event);
                break;
        }
    }
}


