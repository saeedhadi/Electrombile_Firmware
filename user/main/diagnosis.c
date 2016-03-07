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
#include "log.h"


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

    //电池电压介于[36v, 60v]之间
    //FIXME: 根据分压计算区间
    if (voltage < 36 || voltage > 60)
    {
        LOG_ERROR("battery voltage check failed: %d", voltage);
        return EAT_FALSE;
    }

    return EAT_TRUE;
}


/*
 * 检测GSM的信号强度是否 > 13
 */
static eat_bool diag_gsmSignalCheck(void)
{
    int csq = eat_network_get_csq();
    if (csq < 13)
    {
        LOG_ERROR("GSM signal quality not enough: %d", csq);
        return EAT_FALSE;
    }

    return EAT_TRUE;
}

/*
 * 检测433的信号强度
 */
static eat_bool diag_433Check(void)
{
    eat_bool rc;
    u32 voltage;

    rc = eat_get_adc_sync(ADC_433, &voltage);
    if (!rc)
    {
        LOG_ERROR("Get 433 signal quality failed");
        return EAT_FALSE;
    }

    //检查433信号强度是否在[100mv, 300mv]之间
    if (voltage < 100 || voltage > 300)
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
    if (!diag_batterCheck())
    {
        LOG_ERROR("battery check failed!");
        //TODO: light the led

        return EAT_FALSE;
    }

    if (!diag_gsmSignalCheck())
    {
        LOG_ERROR("GSM check failed!");
        //TODO: light the led

        return EAT_FALSE;
    }

    if (!diag_433Check())
    {
        LOG_ERROR("433 check failed!");
        //TODO: light the led

        return EAT_FALSE;
    }

    return EAT_TRUE;
}
