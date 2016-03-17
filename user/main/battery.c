
#include <eat_periphery.h>
#include <eat_timer.h>
#include <math.h>


#include "battery.h"
#include "adc.h"

#define MAX_VLOTAGE_NUM 10

/*  Translate advalue to actual value   */
#define ADvalue_2_Realvalue(x) x*103/3/1000.f //unit mV, 3K & 100k divider


/*
*fun:get dunp battery -x%
*/
char battery_GetBattery(void)
{
    float dum_battery =0;
    u32 tmp_voltage[MAX_VLOTAGE_NUM] = {0};
    u32 voltage = 0;
    int count,rc;
    for(count = 0;count < MAX_VLOTAGE_NUM;count++)//garantee 10 times
    {
        rc = eat_get_adc_sync(ADC_VOLTAGE, &tmp_voltage[count]);
        if(!rc)
        {
            count--;
        }
        eat_sleep(20);
    }

    for(count = 0;count < MAX_VLOTAGE_NUM;count++)
    {
        voltage += tmp_voltage[count];
    }

    voltage /= MAX_VLOTAGE_NUM;

    dum_battery = ADvalue_2_Realvalue(voltage);//Translate to actual value
    return (char)exp((dum_battery -37.873)/2.7927);

}


















