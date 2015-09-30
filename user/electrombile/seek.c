/*
 * seek.c
 *
 *  Created on: 2015/9/17
 *      Author: jk
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "seek.h"
#include "timer.h"
#include "thread.h"
#include "thread_msg.h"
#include "log.h"
#include "data.h"
#include "setting.h"
#include "protocol.h"

#define EAT_ADC0 23
#define EAT_ADC1 24

#define SEEK_INTENSITY_MIN 400
#define SEEK_INTENSITY_MAX 2000

static void seek_timer_handler(void);
static eat_bool seek_getValue(int* value);
static eat_bool seek_sendMsg2Main(MSG_THREAD* msg, u8 len);
static eat_bool seek_sendValue(int value);
static int adcdata0 = 0;

//ADC callback function
void adc_cb_proc(EatAdc_st* adc)
{
    if(adc->pin == EAT_ADC0)
    {
        adcdata0 = adc->v;
        LOG_DEBUG("adcdata0=%d",adcdata0);
    }
}

void app_seek_thread(void *data)
{
    EatEvent_st event;

    LOG_INFO("seek thread start.");
    eat_timer_start(TIMER_SEEK, setting.seek_timer_period);

    while(EAT_TRUE)
    {
        eat_get_event_for_user(THREAD_SEEK, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER :
                switch (event.data.timer.timer_id)
                {
                    case TIMER_SEEK:
                    	//LOG_INFO("TIMER_SEEK expire!");
                        seek_timer_handler();
                        eat_timer_start(event.data.timer.timer_id, setting.seek_timer_period);
                        break;

                    default:
                    	LOG_ERROR("ERR: timer[%d] expire!", event.data.timer.timer_id);
                        break;
                }
                break;

            default:
            	LOG_ERROR("event(%d) not processed", event.event);
                break;

        }
    }
}

static void seek_timer_handler(void)
{
    int ret = EAT_FALSE;
    int value = 0;

    if(EAT_TRUE == seek_fixed())
    {
        LOG_DEBUG("seek fixed.");

        ret = seek_getValue(&value);
        if(EAT_FALSE == ret)
        {
            LOG_ERROR("seek seek_getValue failed.");
            return;
        }

        ret = seek_sendValue(value);
        if(EAT_FALSE == ret)
        {
            LOG_ERROR("seek seek_sendValue failed.");
            return;
        }
    }

    return;
}

static eat_bool seek_getValue(int* value)
{
    eat_adc_get(EAT_ADC0, 0, adc_cb_proc);

    if(0 == adcdata0)
    {
        return EAT_FALSE;
    }
    else if(SEEK_INTENSITY_MIN > adcdata0)
    {
        *value = SEEK_INTENSITY_MIN;
    }
    else if(SEEK_INTENSITY_MAX < adcdata0)
    {
        *value = SEEK_INTENSITY_MAX;
    }
    else
    {
        *value = adcdata0;
    }

    return EAT_TRUE;
}

static eat_bool seek_sendMsg2Main(MSG_THREAD* msg, u8 len)
{
    return sendMsg(THREAD_SEEK, THREAD_MAIN, msg, len);
}

static eat_bool seek_sendValue(int value)
{
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(SEEK_INFO);
    MSG_THREAD* msg = allocMsg(msgLen);
    SEEK_INFO* seek = 0;

    if (!msg)
    {
        LOG_ERROR("alloc msg failed");
        return EAT_FALSE;
    }
    msg->cmd = CMD_THREAD_SEEK;
    msg->length = sizeof(SEEK_INFO);

    seek = (SEEK_INFO*)msg->data;
    seek->intensity = value;

    LOG_DEBUG("send seek: value(%f)", value);
    return seek_sendMsg2Main(msg, msgLen);
}




