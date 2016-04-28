/*
 * led.c
 *
 *  Created on: 2016年3月7日
 *      Author: jk
 */
#include <eat_periphery.h>

#include "log.h"

void LED_on(void)
{
    eat_bool rc;

    rc = eat_gpio_setup(EAT_PIN55_ROW3, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_HIGH);

    if(EAT_FALSE == rc)
    {
        LOG_ERROR("LED_ON error");
    }
}

void LED_off(void)
{
    eat_bool rc;

    rc = eat_gpio_setup(EAT_PIN55_ROW3, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_LOW);

    if(EAT_FALSE == rc)
    {
        LOG_ERROR("LED_OFF error");
    }
}

void LED_fastBlink(void)
{

}

void LED_slowBlink(void)
{

}
