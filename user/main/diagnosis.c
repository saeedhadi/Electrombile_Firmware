/*
 * diagnosis.c
 *
 *  Created on: 2016年3月4日
 *      Author: jk
 */

#include <eat_type.h>
#include <eat_periphery.h>
#include <eat_network.h>

#include "diagnosis.h"
#include "adc.h"
#include "led.h"
#include "log.h"

#define Realvalue_2_ADvalue(x) x*1000*3/103 //unit mV, 3K & 100k divider

/*
 * 检测输入电压范围
 */
static eat_bool diag_batterCheck(void)
{
    eat_bool rc;
    u32 voltage;

    rc = eat_get_adc_sync(ADC_VOLTAGE, &voltage);
    if (!rc)
    {
        LOG_ERROR("Get battery voltage failed");
        return EAT_FALSE;
    }

    //电池电压介于[28v, 85v]之间
    //FIXME: 根据分压计算区间
    if (voltage < Realvalue_2_ADvalue(28) || voltage > Realvalue_2_ADvalue(85))// while testing, 10 and 85 is OK
    {
        LOG_ERROR("battery voltage check failed: %d", voltage);
        return EAT_FALSE;
    }

    return EAT_TRUE;
}

int diag_gsm_get(void)
{
    return eat_network_get_csq();;
}

/*
 * 检测GSM的信号强度是否 > 6
 */
static eat_bool diag_gsmSignalCheck(void)
{
    int csq;
    int count;

    for(count = 0; count < 100; count++)
    {
        if((csq = diag_gsm_get()) <= 0)// Most wait 10s for GSM
        {
            eat_sleep(100);
        }
        else
        {
            break;
        }
    }

    if (csq < 7)
    {
        LOG_ERROR("GSM signal quality not enough: %d", csq);
        return EAT_FALSE;
    }

    return EAT_TRUE;
}

u32 diag_433_get(void)
{
    u32 voltage;
    eat_bool rc = eat_get_adc_sync(ADC_433, &voltage);
    if (!rc)
    {
        LOG_ERROR("Get 433 signal quality failed");
        return 0;
    }

    return voltage;
}


/*
 * 检测433的信号强度
 */
static eat_bool diag_433Check(void)
{
    u32 voltage = diag_433_get();

    //检查433信号强度是否在[100mv, 1000mv]之间
    if (voltage < 100 || voltage > 1000)
    {
        LOG_ERROR("433 signal quality not enough: %d", voltage);
        return EAT_FALSE;
    }

    return EAT_TRUE;
}

/*
 * 自检总入口
 */
eat_bool diag_check(void)
{
    eat_bool ret = EAT_TRUE;

    if (!diag_batterCheck())
    {
        LOG_ERROR("battery check failed!");
        LED_off();
        ret =  EAT_FALSE;
    }

    if (!diag_gsmSignalCheck())
    {
        LOG_ERROR("GSM check failed!");
        LED_off();

        ret = EAT_FALSE;
    }

    if (!diag_433Check())
    {
        LOG_ERROR("433 check failed!");
        LED_off();

        ret = EAT_FALSE;
    }

    if(ret == EAT_TRUE)
    {
        LOG_DEBUG("System check OK");
    }

    return ret;
}
