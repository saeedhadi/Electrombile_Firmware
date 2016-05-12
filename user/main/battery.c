
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

enum
{
    BATTERY_TYPENULL = 0,
    BATTERY_TYPE36 = 36,
    BATTERY_TYPE48 = 48,
    BATTERY_TYPE60 = 60,
    BATTERY_TYPE72 = 72,
};
#define ADvalue_2_Realvalue(x) x*103/3/1000.f //unit mV, 3K & 100k divider
#define Voltage2Percent(x) exp((x-37.873)/2.7927)


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
    percent = percent>100?100:percent;

    if(percent > 30 && percent < 70)
    {
        set_battery_type(battery_type);
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
        voltage = voltage;
    }
    else if(battery_type == BATTERY_TYPE36)
    {
        voltage = voltage*48/36;    //normalizing to 48V
    }
    else
    {
        return 101;         //BATTERY_TYPENULL as 101 > 100
    }

    percent = (int)Voltage2Percent(ADvalue_2_Realvalue(voltage));
    percent = percent>100?100:percent;

    if(percent == 0)//if percent == 0,mostly judge error,set type to default
    {
        set_battery_type(BATTERY_TYPENULL);
    }

    return (u8)percent;
}


u8 battery_get_percent(void)
{
    u8 percent = 0;
    u8 percent_tmp = 0;
    u32 voltage = 0;

    voltage = battery_get_Voltage();

    percent_tmp = battery_Judge_type(voltage);//judge new battery type

    if((percent = battery_getType_percent(voltage)) > 100)//if battery type is not judged , get battery as no type
    {
        percent = percent_tmp;
    }

    return percent;
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
                break;

            case EAT_EVENT_ADC:

                battery_event_adc(&event);

                break;

            case EAT_EVENT_USER_MSG:
                msg = (MSG_THREAD*) event.data.user_msg.data_p;

                switch (msg->cmd)
                {
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


