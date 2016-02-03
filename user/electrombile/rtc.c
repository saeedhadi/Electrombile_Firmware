/*
 * rtc.c
 *
 *  Created on: 2016/2/1
 *      Author: jk
 */

#include <time.h>

#include <eat_interface.h>

#include "rtc.h"
#include "log.h"


//TODO: update the RTC once a day

#define YEAROFFSET 1950


static eat_bool g_isRtcSync = EAT_FALSE;

eat_bool rtc_synced(void)
{
    return g_isRtcSync;
}

//the parameter is in the format of double: 20150327014838.000
void rtc_update(double time)
{
    EatRtc_st rtc = {0};

    g_isRtcSync = EAT_TRUE;


    rtc.year = (short)(time/10000000000) - -YEAROFFSET;
    time -= rtc.year * 10000000000;
    rtc.mon = (short)(time/100000000);
    time -= rtc.mon * 100000000;
    rtc.day = (short)(time/1000000);
    time -= rtc.day * 1000000;
    rtc.hour = (short)(time/10000);
    time -= rtc.hour * 10000;
    rtc.min = (short)(time/100);
    time -= rtc.min * 100;
    rtc.sec = (short)(time);

    LOG_INFO("set RTC to:%4d-%02d-%02d,%02d:%02d:%02d",
            rtc.year+YEAROFFSET,
            rtc.mon,
            rtc.day,
            rtc.hour,
            rtc.min,
            rtc.sec);


    eat_set_rtc(&rtc);
}


time_t rtc_getTimestamp(void)
{
    struct tm stm = {0};
    EatRtc_st rtc = {0};

    eat_get_rtc(&rtc);

    stm.tm_year = rtc.year + YEAROFFSET - 1900;
    stm.tm_mon = rtc.mon - 1;
    stm.tm_mday = rtc.day;
    stm.tm_hour = rtc.hour;
    stm.tm_min = rtc.min;
    stm.tm_sec = rtc.sec;

    return mktime(&stm);
}
