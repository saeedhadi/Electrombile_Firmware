/*
 * watchdog.c
 *
 *  Created on: 2015/6/25
 *      Author: jk
 */

#include <eat_interface.h>
#include <eat_timer.h>

#include "watchdog.h"
#include "log.h"
#include "mileage.h"

#define REBOOT_TIMEOUT  60000   //60s

void startWatchdog(void)
{
	eat_bool rc = eat_watchdog_start(REBOOT_TIMEOUT, 0); //reboot if over time
    if(rc)
    {
        LOG_INFO("open watchdog success.");
    }
    else
    {
        LOG_ERROR("open watchdog fail:%d!", rc);
    }

    return;
}

void stopWatchdog(void)
{
	eat_bool rc = eat_watchdog_stop();
    if(!rc)
    {
        LOG_INFO("stop watchdog success.");
    }
    else
    {
        LOG_ERROR("stop watchdog fail!");
    }

    return;
}

void feedWatchdog(void)
{
	eat_bool rc = eat_watchdog_feed();
    if(!rc )
    {
        LOG_INFO("feed watchdog success.");
    }
    else
    {
        LOG_ERROR("feed watchdog fail:%d!", rc);
    }

    return;
}

