/*
 * seek.c
 *
 *  Created on: 2015/9/17
 *      Author: jk
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "timer.h"
#include "thread.h"
#include "thread_msg.h"
#include "log.h"
#include "data.h"
#include "setting.h"
#include "protocol.h"
#include "seek.h"

static int adcdata0 = 0;
static int adcdata1 = 0;



//ADC callback function
void adc_cb_proc(EatAdc_st* adc)
{
    if(adc->pin == EAT_PIN23_ADC1)
    {
        adcdata0 = adc->v;
        LOG_DEBUG("adcdata0=%d",adcdata0);
    }
    if(adc->pin == EAT_PIN24_ADC2)
    {
        adcdata1 = adc->v;
        LOG_DEBUG("adcdata1=%d",adcdata1);

    }
}

static eat_bool seek_getValue(int* value)
{
    eat_adc_get(EAT_PIN23_ADC1, 0, adc_cb_proc);

    *value = adcdata0;

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

    //TO DO
    //LOG_DEBUG("read EAT_PIN43_GPIO19 = %d.", eat_gpio_read(EAT_PIN43_GPIO19));

    return;
}


void app_seek_thread(void *data)
{
    EatEvent_st event;

    LOG_INFO("seek thread start.");
    eat_timer_start(TIMER_SEEK, setting.seek_timer_period);

    eat_gpio_setup(EAT_PIN43_GPIO19, EAT_GPIO_DIR_INPUT, EAT_GPIO_LEVEL_LOW);
    eat_gpio_setup(EAT_PIN50_NETLIGHT, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_LOW);

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

