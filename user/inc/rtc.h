/*
 * rtc.h
 *
 *  Created on: 2016/2/1
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_RTC_H_
#define USER_ELECTROMBILE_RTC_H_

#include <time.h>

#include <eat_interface.h>

eat_bool rtc_synced(void);

void rtc_setSyncFlag(eat_bool isRtcSync);

void rtc_update(long long time);

time_t rtc_getTimestamp(void);


#endif /* USER_ELECTROMBILE_RTC_H_ */
