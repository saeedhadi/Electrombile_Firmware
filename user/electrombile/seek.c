/*
 * seek.c
 *
 *  Created on: 2015/9/17
 *      Author: jk
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <eat_periphery.h>

#include "timer.h"
#include "thread.h"
#include "thread_msg.h"
#include "log.h"
#include "data.h"
#include "seek.h"

#define ADC0_PERIOD (2000)  //unit: ms

#define EAT_ADC0 EAT_PIN23_ADC1
#define EAT_ADC1 EAT_PIN24_ADC2

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

    LOG_DEBUG("send seek: value(%d)", value);
    return seek_sendMsg2Main(msg, msgLen);
}


void app_seek_thread(void *data)
{
    EatEvent_st event;

    LOG_INFO("seek thread start.");

    eat_gpio_setup(EAT_PIN43_GPIO19, EAT_GPIO_DIR_INPUT, EAT_GPIO_LEVEL_LOW);
    eat_gpio_setup(EAT_PIN55_ROW3, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_HIGH);

    eat_adc_get(EAT_ADC0, ADC0_PERIOD, NULL);

    while(EAT_TRUE)
    {
        eat_get_event_for_user(THREAD_SEEK, &event);
        switch(event.event)
        {
             case EAT_EVENT_ADC:
                if (event.data.adc.pin == EAT_ADC0)
                {
                    if (isSeekMode())
                    {
                        unsigned int value = event.data.adc.v;
                        LOG_INFO("EAT_ADC0 = %d", value);
                        seek_sendValue(value);
                    }
                }
                else
                {
                    LOG_ERROR("un-processed adc pin %d", event.data.adc.pin);
                }
                break;

            default:
                LOG_ERROR("event(%d) not processed", event.event);
                break;

        }
    }
}

