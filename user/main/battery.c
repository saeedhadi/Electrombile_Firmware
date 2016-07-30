
#include <eat_periphery.h>
#include <eat_timer.h>
#include <eat_interface.h>


#include <math.h>

#include "thread_msg.h"
#include "thread.h"
#include "battery.h"
#include "setting.h"
#include "data.h"
#include "log.h"
#include "adc.h"
#include "mem.h"
#include "timer.h"

enum
{
    BATTERY_TYPENULL = 0,
    BATTERY_TYPE36 = 36,
    BATTERY_TYPE48 = 48,
    BATTERY_TYPE60 = 60,
    BATTERY_TYPE72 = 72,
};

enum
{
    BATTERY_ALARM_NULL  = 0,
    BATTERY_ALARM_50    = 4,
    BATTERY_ALARM_30    = 5,
};


#define MAX_PERCENT_NUM 100
#define BATTERY_TIMER_PEROID (10*60*1000)   //10mins check once


#define ADvalue_2_Realvalue(x) x*103/3/1000.f //unit mV, 3K & 100k divider
#define Voltage2Percent(x) exp((x-37.873)/2.7927)

static u32 BatteryVoltage[MAX_VLOTAGE_NUM] = {0};

/*
*set the battery's voltage
*/
static void battery_store_voltage(u32 voltage)
{
    static int count = 0;

    if(count >= MAX_VLOTAGE_NUM)
    {
        count = 0;
    }

    BatteryVoltage[count++] = voltage;
}

/*
* get the battery's voltage
*/
static u32 battery_get_Voltage(void)
{
    u32 voltage = 0;
    int count;

    for(count = 0;count < MAX_VLOTAGE_NUM;count++)
    {
        voltage += BatteryVoltage[count];
    }

    voltage /= MAX_VLOTAGE_NUM;

    return voltage;
}


/*
*fun:judge if there is new battery type and return battery percent
*/
static u8 battery_Judge_type(u32 voltage)
{
    float realVoltage = ADvalue_2_Realvalue(voltage);
    u8 battery_type = BATTERY_TYPENULL;
    int percent = 0;//the result of the fumula may be larger than 256

    if(realVoltage > 64 )
    {
        voltage = voltage*48/72;    //normalizing to 48V
        battery_type = BATTERY_TYPE72;
    }
    else if(realVoltage > 52)
    {
        voltage = voltage*48/60;    //normalizing to 48V
        battery_type = BATTERY_TYPE60;
    }
    else if(realVoltage > 40)
    {
        voltage = voltage;
        battery_type = BATTERY_TYPE48;
    }
    else if(realVoltage > 28)
    {
        voltage = voltage*48/36;    //normalizing to 48V
        battery_type = BATTERY_TYPE36;
    }

    percent = (int)Voltage2Percent(ADvalue_2_Realvalue(voltage));
    percent = percent>MAX_PERCENT_NUM?MAX_PERCENT_NUM:percent;

    if(battery_type == get_battery_type())
    {
        LOG_DEBUG("battery type OK: %d",battery_type);
        return (u8)percent;
    }

    if(60 < percent)
    {
        if(battery_type != get_battery_test_type())
        {
            LOG_DEBUG("set battery flag: %d",battery_type);
            set_battery_flag(EAT_TRUE, battery_type);
        }

        return (u8)percent;
    }

    if(40 > percent)
    {
        if(get_battery_flag() && battery_type == get_battery_test_type())
        {
            LOG_DEBUG("set battery type: %d",battery_type);
            set_battery_type(battery_type);
        }
        else
        {
            LOG_DEBUG("reset battery type: %d",battery_type);
            set_battery_flag(EAT_FALSE, NULL);
        }
    }

    return (u8)percent;
}

/*
*fun:reference battery-type, return battery percent
*/
static u8 battery_getType_percent(u32 voltage)
{
    int percent = 0;
    u8 battery_type = get_battery_type();

    if(battery_type == BATTERY_TYPE72)
    {
        voltage = voltage*48/72;    //normalizing to 48V
    }
    else if(battery_type == BATTERY_TYPE60)
    {
        voltage = voltage*48/60;    //normalizing to 48V
    }
    else if(battery_type == BATTERY_TYPE48)
    {
        voltage = voltage;          //normalizing to 48V
    }
    else if(battery_type == BATTERY_TYPE36)
    {
        voltage = voltage*48/36;    //normalizing to 48V
    }
    else
    {
        return (MAX_PERCENT_NUM + 1); //BATTERY_TYPENULL as MAX_PERCENT_NUM + 1 > MAX_PERCENT_NUM
    }

    percent = (int)Voltage2Percent(ADvalue_2_Realvalue(voltage));
    percent = percent>MAX_PERCENT_NUM?MAX_PERCENT_NUM:percent;

    return (u8)percent;
}


static u8 battery_get_percent(void)
{
    u8 percent = 0;
    u8 percent_tmp = 0;
    u32 voltage = 0;

    voltage = battery_get_Voltage();

    percent_tmp = battery_Judge_type(voltage);//judge new battery type

    if((percent = battery_getType_percent(voltage)) > MAX_PERCENT_NUM)//if battery type is not judged , get battery as no type
    {
        percent = percent_tmp;
    }

    return percent;
}

static u8 battery_get_miles(void)
{
    return 0;
}

/*
*func:check battery 10mins once while not moved,if battery low,alarm:BATTERY_ALARM_50, BATTERY_ALARM_30
*/
static char battery_percent_check(void)
{
    static char state = BATTERY_ALARM_NULL;
    u8 percent = battery_get_percent();

    if(70 < percent)    //battery > 70, assume as charge, reset and wait for reducing to 50
    {
        state = BATTERY_ALARM_NULL;
    }
    else if(50 > percent)
    {
        if(30 < percent && state != BATTERY_ALARM_50 && state != BATTERY_ALARM_30)//30 < battery <50,alarm once,and wait for reducing to 30
        {
            return state = BATTERY_ALARM_50;
        }
        else if(state != BATTERY_ALARM_30)//30 < battery,alarm once,and do nothing
        {
            return state = BATTERY_ALARM_30;
        }
    }

    return BATTERY_ALARM_NULL;
}

static int battery_alarm_handler(void)
{
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(ALARM_INFO);
    MSG_THREAD *msg = NULL;
    ALARM_INFO *alarmType = NULL;
    char alarm_type = battery_percent_check();

    if(Vibration_isMoved())
    {
        return 0;
    }

    if(alarm_type == BATTERY_ALARM_NULL)
    {
        return 0;
    }

    msg = allocMsg(msgLen);
    alarmType = (ALARM_INFO*)msg->data;

    msg->cmd = CMD_THREAD_ALARM;
    msg->length = sizeof(ALARM_INFO);
    alarmType->alarm_type = alarm_type;

    LOG_DEBUG("battery alarm:cmd(%d),length(%d),data(%d)", msg->cmd, msg->length, alarm_type);

    return sendMsg(THREAD_MAIN, msg, msgLen);

}


static int battery_get_handler(u8 cmd)
{
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(BATTERY_INFO);
    MSG_THREAD *msg = allocMsg(msgLen);
    BATTERY_INFO *msg_state = 0;

    if (!msg)
    {
        LOG_ERROR("alloc battery msg failed!");
        return EAT_FALSE;
    }

    msg->cmd = cmd;
    msg->length = sizeof(BATTERY_INFO);

    msg_state = (BATTERY_INFO*)msg->data;
    msg_state->percent= battery_get_percent();
    msg_state->miles = battery_get_miles();

    LOG_DEBUG("send battery state msg to Main_thread:%d,%d",msg_state->percent,msg_state->miles);
    sendMsg(THREAD_MAIN, msg, msgLen);

    return 0;

}

static int battery_get_msg(const MSG_THREAD* thread_msg)
{
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(BATTERY_GET_INFO);
    MANAGERSEQ_INFO *main_data = (MANAGERSEQ_INFO*)thread_msg->data;
    MSG_THREAD *msg = allocMsg(msgLen);
    BATTERY_GET_INFO *msg_state = (BATTERY_GET_INFO*)msg->data;

    if (!msg)
    {
        LOG_ERROR("alloc battery msg failed!");
        return EAT_FALSE;
    }

    msg->cmd = thread_msg->cmd;
    msg->length = sizeof(BATTERY_GET_INFO);

    msg_state = (BATTERY_GET_INFO*)msg->data;
    msg_state->percent= battery_get_percent();
    msg_state->miles = battery_get_miles();
    msg_state->managerSeq = main_data->managerSeq;

    LOG_DEBUG("send battery msg to Main_thread:%d,%d",msg_state->percent,msg_state->miles);
    sendMsg(THREAD_MAIN, msg, msgLen);

    return 0;

}



/*
*fun:event adc proc
*/
static void battery_event_adc(EatEvent_st *event)
{
    if(event->data.adc.pin == ADC_VOLTAGE)
    {
        battery_store_voltage(event->data.adc.v);
    }
    else
    {
        LOG_ERROR("ADC_433 = %d",event->data.adc.v);
    }
}

void app_battery_thread(void *data)
{
	EatEvent_st event;
    MSG_THREAD* msg = 0;

	LOG_INFO("battery thread start.");

    while(!eat_adc_get(ADC_VOLTAGE, ADC_VOLTAGE_PERIOD, NULL))
    {
        eat_sleep(100);//if failed ,try again after 100ms
    }

	LOG_INFO("TIMER_BATTERY_CHECK start.");
    eat_timer_start(TIMER_BATTERY_CHECK,BATTERY_TIMER_PEROID);

	while(EAT_TRUE)
	{
        eat_get_event_for_user(THREAD_BATTERY, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER:
                switch (event.data.timer.timer_id)
                {
                    case TIMER_BATTERY_CHECK:
                        battery_alarm_handler();
                        eat_timer_start(event.data.timer.timer_id,BATTERY_TIMER_PEROID);
                        break;

                    default:
                        LOG_ERROR("timer(%d) expire!", event.data.timer.timer_id);
                        break;
                }
                break;

            case EAT_EVENT_ADC:

                battery_event_adc(&event);

                break;

            case EAT_EVENT_USER_MSG:
                msg = (MSG_THREAD*) event.data.user_msg.data_p;

                switch (msg->cmd)
                {
                    case CMD_THREAD_BATTERY:
                        battery_get_handler(msg->cmd);
                        break;

                    case CMD_THREAD_BATTERY_INFO:
                        battery_get_handler(msg->cmd);
                        break;

                    case CMD_THREAD_BATTERY_GET:
                        battery_get_msg(msg);
                        break;

                    default:
                        LOG_ERROR("cmd(%d) not processed!", msg->cmd);
                        break;
                }
                freeMsg(msg);
                break;

            default:
            	LOG_ERROR("event(%d) not processed!", event.event);
                break;
        }
    }
}


