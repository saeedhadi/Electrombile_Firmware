/*
 * seek.c
 *
 *  Created on: 2015/9/17
 *      Author: jk
 */

#include <eat_periphery.h>


#include "log.h"
#include "seek.h"
#include "adc.h"
#include "request.h"
#include "timer.h"

#define SEEK_AUTO_OFF_PERIOD    30 * 1000   //30 seconds

void seek_initial(void)
{
    eat_gpio_setup(EAT_PIN43_GPIO19, EAT_GPIO_DIR_INPUT, EAT_GPIO_LEVEL_LOW);
    eat_gpio_setup(EAT_PIN55_ROW3, EAT_GPIO_DIR_OUTPUT, EAT_GPIO_LEVEL_HIGH);

}

int seek_proc(unsigned int value)
{
    return cmd_Seek(value);
}


void setSeekMode(eat_bool fixed)
{
    if (fixed)
    {
        eat_adc_get(ADC_433, ADC_433_PERIOD, NULL);

        //启动定时器，SEEK_AUTO_OFF_PERIOD 后自动退出找车模式，防止客户端不下发关闭找车模式命令
        eat_timer_start(TIMER_SEEKAUTOOFF, SEEK_AUTO_OFF_PERIOD);
    }
    else
    {
        eat_adc_get(ADC_433, ADC_PERIOD_READ_ONCE, NULL);
    }
}
