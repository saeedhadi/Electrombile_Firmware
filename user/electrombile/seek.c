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

void delay_50(void) 
{
    u8 i,j;
	for(i=5;i>0;i--)
	for(j=0x12;j>0;j--);
}

/*-------------------------------250us精确延时---------------------------------*/
void delay_600(void) 
{
    u8 i,j;
	for(i=50;i>0;i--)
	for(j=0x17;j>0;j--);
}

/*-------------------------------131ms精确延时--------------------------------*/
void delay_131(void) 
{
    u8 i,j;
	for(i=0xfe;i>0;i--)
	for(j=0xfe;j>0;j--);
}
static void seek_timer_handler(void)
{
    int ret = EAT_FALSE;
    int value = 0;
    u8 i,j;
    EatGpioLevel_enum BitState_2; 
    u8  ReadCode[3],GetCode;

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
    if(0)
 /*   {
          eat_gpio_write(50,EAT_GPIO_LEVEL_HIGH);
          delay_200();
          eat_gpio_write(50,EAT_GPIO_LEVEL_LOW);
          delay_200();
          eat_gpio_write(50,EAT_GPIO_LEVEL_HIGH);
          delay_200();
          eat_gpio_write(50,EAT_GPIO_LEVEL_LOW);
          delay_200();
          
    }
*/    {   
//////////////////////////////找起始位//////////////////////////////////////////
	
       value=0;
ks:	while(eat_gpio_read(EAT_PIN43_GPIO19)==1);
	for(i=100;i>0;i--) 					//重复20次，检测在3750微秒内出现高电平就退出解码程序
	{    
               delay_50(); 
		if(eat_gpio_read(EAT_PIN43_GPIO19)==1){
               value ++;
               if(value<100)
               goto ks;
               else
               {
                    LOG_DEBUG("getcode error = no start,value=%d",value);
                    return;
               }
         }
	}                                   
	
	while(eat_gpio_read(EAT_PIN43_GPIO19)==0);

///////////////////////////////接收数据///////////////////////////////////////////
    

	for(j=0;j<3;j++)                    
	{
		for(i=0;i<8;i++)
		{ 
            delay_600();
			BitState_2=eat_gpio_read(EAT_PIN43_GPIO19);		
			ReadCode[j]=ReadCode[j]<<1;
			ReadCode[j]=ReadCode[j]|BitState_2; 
			if(eat_gpio_read(EAT_PIN43_GPIO19)==1) while(eat_gpio_read(EAT_PIN43_GPIO19)==1);
			while(eat_gpio_read(EAT_PIN43_GPIO19)==0);
		   				
		}
	}
////////////////////////////////校验及运算数据/////////////////////////////////////
    for(i=0;i<3;i++)
    {
       if((~(ReadCode[i]|0x55))&((ReadCode[i]&0x55)<<1)!=0x00)   {
                LOG_DEBUG("getcode error = error data");
                    return;
         }    //校验
    } 
    GetCode=0x00;
    for(i=0;i<8;i++)
    {
      GetCode |=((ReadCode[2]>>i)&(ReadCode[2]>>(i+1))&0x01<<i);             //运算
    }
    LOG_DEBUG("GetCode = %d",GetCode);
        
    }

    //TO DO
    //LOG_DEBUG("read EAT_PIN43_GPIO19 = %d.", eat_gpio_read(EAT_PIN43_GPIO19));

    return;
}

static eat_bool seek_getValue(int* value)
{
    eat_adc_get(EAT_ADC0, 0, adc_cb_proc);
/*
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
*/
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




