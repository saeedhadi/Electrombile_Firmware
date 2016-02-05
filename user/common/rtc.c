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

#define YEAROFFSET 1954


static eat_bool g_isRtcSync = EAT_FALSE;

eat_bool rtc_synced(void)
{
    return g_isRtcSync;
}

//the parameter is in the format: 20150327014838
void rtc_update(long long time)
{
    EatRtc_st rtc = {0};

    rtc.year = time / 10000000000 - YEAROFFSET;
    time %= 10000000000;

    //SIM808启动后GPS未定位时的默认时间为19800106000012,rtc.year存储的是和YEAROFFSET差值,如果大于30则说明GPS时间同步了
    if (rtc.year > 30)
    {
        g_isRtcSync = EAT_TRUE;
    }

    rtc.mon = time / 100000000;
    time %= 100000000;

    rtc.day = time / 1000000;
    time %= 1000000;

    rtc.hour = time / 10000;
    time %= 10000;

    rtc.min = time / 100;
    time %= 100;

    rtc.sec = time;

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
