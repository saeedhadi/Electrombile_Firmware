/*
 * sms.c
 *
 *  Created on: 2015/6/24
 *      Author: jk
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "eat_modem.h"
#include "eat_interface.h"
#include "eat_uart.h"
#include "eat_sms.h"
#include "setting.h"
#include "sms.h"
#include "log.h"
#include "thread.h"
#include "tool.h"
#include "version.h"
#include "timer.h"

//because #include "mileage" can't pass compile , so define again there
#define MILEAGEFILE_NAME   L"C:\\mileage.txt"

static eat_bool ResetFlag = EAT_FALSE;


static eat_sms_flash_message_cb(EatSmsReadCnf_st smsFlashMessage)
{
    u8 format = 0;

    LOG_DEBUG("flash message.");
    eat_get_sms_format(&format);
    if(1 == format)//TEXTģʽ
    {
        LOG_DEBUG("recv TEXT sms.");
        LOG_DEBUG("msg=%s.",smsFlashMessage.data);
        LOG_DEBUG("datetime=%s.",smsFlashMessage.datetime);
        LOG_DEBUG("name=%s.",smsFlashMessage.name);
        LOG_DEBUG("status=%d.",smsFlashMessage.status);
        LOG_DEBUG("len=%d.",smsFlashMessage.len);
        LOG_DEBUG("number=%s.",smsFlashMessage.number);
    }
    else//PDUģʽ
    {
        LOG_DEBUG("recv PDU sms.");
        LOG_DEBUG("msg=%s",smsFlashMessage.data);
        LOG_DEBUG("len=%d",smsFlashMessage.len);
    }
}

static void sms_version_proc(u8 *p, u8 *number)
{
    unsigned char *ptr1;
    unsigned char ack_message[64]={0};

    ptr1 = tool_StrstrAndReturnEndPoint(p, "VERSION?");
    if(NULL != ptr1)
    {
        sprintf(ack_message, "VER:%s\r\nCORE:%s", VERSION_STR, eat_get_version());
        eat_send_text_sms(number, ack_message);
    }

    return;
}

static void sms_factory_proc(u8 *p, u8 *number)
{
    unsigned char *ptr1;
    char ack_message[64] = {0};
    eat_fs_error_enum fs_Op_ret;


    ptr1 = tool_StrstrAndReturnEndPoint(p, "Factory");
    if(NULL != ptr1)
    {

        sprintf(ack_message, "Factory default ok");

        LOG_DEBUG("send reply sms to %d:%s",number,ack_message);
        eat_send_text_sms(number, ack_message);

        fs_Op_ret = (eat_fs_error_enum)eat_fs_Delete(SETTINGFILE_NAME);

        if(EAT_FS_NO_ERROR != fs_Op_ret && EAT_FS_FILE_NOT_FOUND != fs_Op_ret)
        {
            LOG_ERROR("Delete settingfile Fail,and Return Error is %d",fs_Op_ret);
        }
        else
        {
            LOG_DEBUG("Delete settingfile Success");
        }


        fs_Op_ret = (eat_fs_error_enum)eat_fs_Delete(LOGFILE_NAME);

        if(EAT_FS_NO_ERROR != fs_Op_ret && EAT_FS_FILE_NOT_FOUND != fs_Op_ret)
        {
            LOG_ERROR("Delete logfile Fail,and Return Error is %d",fs_Op_ret);
        }
        else
        {
            LOG_DEBUG("Delete logfile Success");
        }


        fs_Op_ret = (eat_fs_error_enum)eat_fs_Delete(MILEAGEFILE_NAME);

        if(EAT_FS_NO_ERROR != fs_Op_ret && EAT_FS_FILE_NOT_FOUND != fs_Op_ret)
        {
            LOG_ERROR("Delete mileagefile Fail,and Return Error is %d",fs_Op_ret);
        }
        else
        {
            LOG_DEBUG("Delete mileagefile Success");
        }

        // if time is less , send text will send fail,so proposal not to reply sms there
        eat_sleep(10*1000);

        eat_reset_module();

    }
}


static void sms_reboot_proc(u8 *p, u8 *number)
{
    unsigned char *ptr1;
    char ack_message[64] = {0};


    ptr1 = tool_StrstrAndReturnEndPoint(p, "RESET");
    if(NULL != ptr1)
    {

        sprintf(ack_message, "Reset ok");

        LOG_DEBUG("send reply sms to %d:%s",number,ack_message);
        eat_send_text_sms(number, ack_message);
        LOG_DEBUG("ready to reboot...");

        // if time is less , send text will send fail,so proposal not to reply sms there
        eat_sleep(10*1000);

        eat_reset_module();
    }
}


static void sms_server_proc(u8 *p, u8 *number)
{
    unsigned char *ptr1;
    char ack_message[64] = {0};
    char domainORip[MAX_DOMAIN_NAME_LEN] = {0};
    char domain[MAX_DOMAIN_NAME_LEN] = {0};
    u32 ip[4] = {0};
    u32 port = 0;
    int count = 0;

    ptr1 = tool_StrstrAndReturnEndPoint(p, "SERVER?");
    if(NULL != ptr1)
    {
        if(setting.addr_type == ADDR_TYPE_IP)
        {
            sprintf(ack_message, "SERVER %d.%d.%d.%d:%d",setting.ipaddr[0],setting.ipaddr[1],setting.ipaddr[2],setting.ipaddr[3],setting.port);
        }
        else if(setting.addr_type == ADDR_TYPE_DOMAIN)
        {
            sprintf(ack_message, "SERVER %s:%d",setting.domain,setting.port);
        }

        LOG_DEBUG("send reply sms to %d:%s",number,ack_message);
        eat_send_text_sms(number, ack_message);
    }

    ptr1 = tool_StrstrAndReturnEndPoint(p, "SERVER ");
    if(NULL != ptr1)
    {
        count = sscanf(ptr1, "%[^:]:%u", domainORip, &port);
        if(2 == count)
        {
            count = sscanf(domainORip, "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]);
            if(4 == count)
            {
                //validity check
                if(ip[0] <= 255 && ip[1] <= 255 && ip[2] <= 255 && ip[3] <= 255)
                {
                    //domainORip is ip
                    setting.addr_type = ADDR_TYPE_IP;
                    setting.ipaddr[0] = (u8)ip[0];
                    setting.ipaddr[1] = (u8)ip[1];
                    setting.ipaddr[2] = (u8)ip[2];
                    setting.ipaddr[3] = (u8)ip[3];
                    setting.port = (u16)port;

                    setting_save();

                    //eat_reset_module();//TO DO
                    ResetFlag = EAT_TRUE;

                    //return ok
                    sprintf(ack_message, "%s:%u OK", domainORip, port);
                }
                else
                {
                    //return error
                    sprintf(ack_message, "%s:%u ERROR", domainORip, port);
                }
            }
            else
            {
                //validity check; a-zA-Z0-9 not work
                count = sscanf(domainORip, "%[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.]", domain);
                if(1 == count)
                {
                    //domainORip is domain
                    setting.addr_type = ADDR_TYPE_DOMAIN;
                    strcpy(setting.domain, domainORip);
                    setting.port = (u16)port;

                    setting_save();

                    //eat_reset_module();//TO DO
                    ResetFlag = EAT_TRUE;

                    //return ok
                    sprintf(ack_message, "%s:%u OK", domain, port);
                }
                else
                {
                    //return error
                    sprintf(ack_message, "%s:%u ERROR", domainORip, port);
                }
            }
        }
        else
        {
            sprintf(ack_message, "%s ERROR", ptr1);
        }

        LOG_DEBUG("send reply sms to %d:%s",number,ack_message);
        eat_send_text_sms(number, ack_message);
    }

    return;
}

//TODO: fix the gps upload timer
static void sms_timer_proc(u8 *p, u8 *number)
{
    unsigned char *ptr1;
    char ack_message[64] = {0};
    u32 timer_period = 0;
    int count = 0;

    ptr1 = tool_StrstrAndReturnEndPoint(p, "TIMER?");
    if(NULL != ptr1)
    {
        sprintf(ack_message, "TIMER:%u", (30 * 1000 / 1000));

        LOG_DEBUG("send reply sms to %d:%s",number,ack_message);
        eat_send_text_sms(number, ack_message);
    }

    ptr1 = tool_StrstrAndReturnEndPoint(p, "TIMER ");
    if(NULL != ptr1)
    {
        count = sscanf(ptr1, "%u", &timer_period);
        if(1 == count)
        {
            if(0 == timer_period)
            {
                sprintf(ack_message, "SET TIMER to 0 OK");
            }
            else if(timer_period <= 10)
            {
//                setting.gps_send_timer_period = 10000;

                setting_save();

                sprintf(ack_message, "SET TIMER to 10 OK");
            }
            else if(timer_period >= 21600)
            {
//                setting.gps_send_timer_period = 21600 * 1000;

                setting_save();

                sprintf(ack_message, "SET TIMER to 21600 OK");
            }
            else
            {
//                setting.gps_send_timer_period = timer_period * 1000;

                setting_save();

                sprintf(ack_message, "SET TIMER to %d OK", timer_period);
            }
        }
        else
        {
            sprintf(ack_message, "SET TIMER to %s ERROR", ptr1);
        }

        LOG_DEBUG("send reply sms to %d:%s",number,ack_message);
        eat_send_text_sms(number, ack_message);
    }

    return;
}

static void eat_sms_read_cb(EatSmsReadCnf_st smsReadCnfContent)
{
    u8 format = 0;
    unsigned char *p = smsReadCnfContent.data;
    //unsigned char *ptr1;
    //unsigned char *ptr2;
    //char ack_message[64]={0};
    //int count = 0;
    //u8 ipaddr[4] = {0};
    //int port = 0;
    //s8 domain[MAX_DOMAIN_NAME_LEN];
    //u32 timer_period = 0;

    LOG_DEBUG("new message.");
    eat_get_sms_format(&format);
    if(1 == format)//TEXTģʽ
    {
        LOG_DEBUG("recv TEXT sms.");

        sms_version_proc(p, smsReadCnfContent.number);
        sms_server_proc(p, smsReadCnfContent.number);
        sms_timer_proc(p, smsReadCnfContent.number);
        sms_reboot_proc(p, smsReadCnfContent.number);
        sms_factory_proc(p, smsReadCnfContent.number);
    }
    else//PDUģʽ
    {
        LOG_DEBUG("recv PDU sms.");
    }
}

static void eat_sms_delete_cb(eat_bool result)
{
    LOG_DEBUG("result=%d.", result);
}

static void eat_sms_send_cb(eat_bool result)
{
    LOG_DEBUG("result=%d.", result);

    if(EAT_TRUE == ResetFlag)
    {
        ResetFlag = EAT_FALSE;
        eat_reset_module();
    }
}

static eat_sms_new_message_cb(EatSmsNewMessageInd_st smsNewMessage)
{
    LOG_DEBUG("smsNewMessage.index=%d.", smsNewMessage.index);
    eat_read_sms(smsNewMessage.index, eat_sms_read_cb);
}

static void eat_sms_ready_cb(eat_bool result)
{
    LOG_DEBUG("result=%d.", result);
}

void app_sms_thread(void *data)
{
    EatEvent_st event;

    LOG_DEBUG("SMS thread start.");

    eat_set_sms_operation_mode(EAT_TRUE);//set sms operation as API mode
    eat_set_sms_format(EAT_TRUE);//set sms format as TEXT mode
    //eat_set_sms_cnmi(0,0,0,0,0);//set sms cnmi parameter
    //eat_set_sms_sc("+8613800290500");//set center number
    //eat_set_sms_storage(EAT_ME, EAT_ME, EAT_ME);//set sms storage type

    eat_sms_register_new_message_callback(eat_sms_new_message_cb);
    eat_sms_register_flash_message_callback(eat_sms_flash_message_cb);
    eat_sms_register_send_completed_callback(eat_sms_send_cb);
    eat_sms_register_sms_ready_callback(eat_sms_ready_cb);

    while(EAT_TRUE)
    {
        eat_get_event_for_user(TRHEAD_SMS, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER :
                switch (event.data.timer.timer_id)
                {
                    default:
                    	LOG_ERROR("ERR: timer[%d] not handle!", event.data.timer.timer_id);
                        break;
                }
                break;

            case EAT_EVENT_MDM_READY_RD:
                break;

            case EAT_EVENT_MDM_READY_WR:
                break;

            case EAT_EVENT_USER_MSG:
                break;

            default:
                break;

        }
    }
}

