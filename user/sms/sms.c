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
#include "version.h"
#include "fs.h"
#include "utils.h"
#include "thread_msg.h"

static eat_bool ResetFlag = EAT_FALSE;

static void sms_version_proc(u8 *p, u8 *number)
{
    MSG_THREAD *msg = NULL;
    SMS_SEND_INFO *data = NULL;
    u8 msgLen = 0;
    u8 *ptr1;
    u8 ack_message[ACK_MESSAGE_LEN]={0};

    ptr1 = string_bypass(p, "VERSION?");
    if(NULL != ptr1)
    {
        snprintf(ack_message, ACK_MESSAGE_LEN, "VER:%s\r\nCORE:%s", VERSION_STR, eat_get_version());
        msgLen = sizeof(MSG_THREAD) + sizeof(SMS_SEND_INFO) + strlen(ack_message);
        msg = allocMsg(msgLen);
        if (!msg)
        {
            LOG_ERROR("alloc sms msg failed!");
            return;
        }

        msg->cmd = CMD_THREAD_SMS;
        msg->length = sizeof(SMS_SEND_INFO) + strlen(ack_message);

        data = (SMS_SEND_INFO*)msg->data;
        strncpy(data->number,number,TEL_NO_LENGTH + 1);
        data->type = SMS_SEND_DIRECT;
        data->smsLen = strlen(ack_message);
        strncpy(data->content,ack_message,ACK_MESSAGE_LEN);

        sendMsg(THREAD_MAIN, msg, msgLen);
    }

    return;
}

static void sms_factory_proc(u8 *p, u8 *number)
{
    MSG_THREAD *msg = NULL;
    SMS_SEND_INFO *data = NULL;
    u8 msgLen = 0;
    u8 *ptr1;
    u8 ack_message[ACK_MESSAGE_LEN]={0};
    int rc = 0;

    ptr1 = string_bypass(p, "Factory");
    if(NULL != ptr1)
    {
        rc = fs_factory();
        if(0 > rc)
        {
            snprintf(ack_message, ACK_MESSAGE_LEN, "Factory default error");
        }
        else
        {
            snprintf(ack_message, ACK_MESSAGE_LEN, "Factory default ok");
        }
        msgLen = sizeof(MSG_THREAD) + sizeof(SMS_SEND_INFO) + strlen(ack_message);
        msg = allocMsg(msgLen);
        if (!msg)
        {
            LOG_ERROR("alloc sms msg failed!");
            return;
        }

        msg->cmd = CMD_THREAD_SMS;
        msg->length = sizeof(SMS_SEND_INFO) + strlen(ack_message);

        data = (SMS_SEND_INFO*)msg->data;
        strncpy(data->number,number,TEL_NO_LENGTH + 1);
        data->type = SMS_SEND_DIRECT;
        data->smsLen = strlen(ack_message);
        strncpy(data->content,ack_message,ACK_MESSAGE_LEN);

        sendMsg(THREAD_MAIN, msg, msgLen);

        ResetFlag = EAT_TRUE;
    }

    return;
}


static void sms_reboot_proc(u8 *p, u8 *number)
{
    MSG_THREAD *msg = NULL;
    SMS_SEND_INFO *data = NULL;
    u8 msgLen = 0;
    u8 *ptr1;
    u8 ack_message[ACK_MESSAGE_LEN]={0};

    ptr1 = string_bypass(p, "RESET");
    if(NULL != ptr1)
    {
        snprintf(ack_message, ACK_MESSAGE_LEN, "Reset ok");
        msgLen = sizeof(MSG_THREAD) + sizeof(SMS_SEND_INFO) + strlen(ack_message);
        msg = allocMsg(msgLen);
        if (!msg)
        {
            LOG_ERROR("alloc sms msg failed!");
            return;
        }

        msg->cmd = CMD_THREAD_SMS;
        msg->length = sizeof(SMS_SEND_INFO) + strlen(ack_message);

        data = (SMS_SEND_INFO*)msg->data;
        strncpy(data->number,number,TEL_NO_LENGTH + 1);
        data->type = SMS_SEND_DIRECT;
        data->smsLen = strlen(ack_message);
        strncpy(data->content,ack_message,ACK_MESSAGE_LEN);

        sendMsg(THREAD_MAIN, msg, msgLen);

        ResetFlag = EAT_TRUE;
    }

    return;
}


static void sms_server_proc(u8 *p, u8 *number)
{
    MSG_THREAD *msg = NULL;
    SMS_SEND_INFO *data = NULL;
    u8 msgLen = 0;
    u8 *ptr1;
    u8 ack_message[ACK_MESSAGE_LEN]={0};
    char domainORip[MAX_DOMAIN_NAME_LEN] = {0};
    char domain[MAX_DOMAIN_NAME_LEN] = {0};
    u32 ip[4] = {0};
    u32 port = 0;
    int count = 0;

    ptr1 = string_bypass(p, "SERVER?");
    if(NULL != ptr1)
    {
        if(setting.addr_type == ADDR_TYPE_IP)
        {
            snprintf(ack_message, ACK_MESSAGE_LEN, "SERVER %d.%d.%d.%d:%d",setting.ipaddr[0],setting.ipaddr[1],setting.ipaddr[2],setting.ipaddr[3],setting.port);
        }
        else if(setting.addr_type == ADDR_TYPE_DOMAIN)
        {
            snprintf(ack_message, ACK_MESSAGE_LEN, "SERVER %s:%d",setting.domain,setting.port);
        }
    }

    ptr1 = string_bypass(p, "SERVER ");
    if(NULL != ptr1)
    {
        count = sscanf(ptr1, "%[^:]:%u", domainORip, &port);
        if(2 == count)
        {
            count = sscanf(domainORip, "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]);
            if(4 == count)
            {
                if(ip[0] <= 255 && ip[1] <= 255 && ip[2] <= 255 && ip[3] <= 255)
                {
                    setting.addr_type = ADDR_TYPE_IP;
                    setting.ipaddr[0] = (u8)ip[0];
                    setting.ipaddr[1] = (u8)ip[1];
                    setting.ipaddr[2] = (u8)ip[2];
                    setting.ipaddr[3] = (u8)ip[3];
                    setting.port = (u16)port;

                    setting_save();
                    ResetFlag = EAT_TRUE;

                    snprintf(ack_message, ACK_MESSAGE_LEN, "%s:%u OK", domainORip, port);
                }
                else
                {
                    snprintf(ack_message, ACK_MESSAGE_LEN, "%s:%u ERROR", domainORip, port);
                }
            }
            else
            {
                count = sscanf(domainORip, "%[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.]", domain);
                if(1 == count)
                {
                    setting.addr_type = ADDR_TYPE_DOMAIN;
                    strncpy(setting.domain, domainORip,MAX_DOMAIN_NAME_LEN);
                    setting.port = (u16)port;

                    setting_save();
                    ResetFlag = EAT_TRUE;

                    snprintf(ack_message, ACK_MESSAGE_LEN, "%s:%u OK", domain, port);
                }
                else
                {
                    snprintf(ack_message, ACK_MESSAGE_LEN, "%s:%u ERROR", domainORip, port);
                }
            }
        }
        else
        {
            snprintf(ack_message, ACK_MESSAGE_LEN, "%s ERROR", ptr1);
        }
    }

    msgLen = sizeof(MSG_THREAD) + sizeof(SMS_SEND_INFO) + strlen(ack_message);
    msg = allocMsg(msgLen);
    if (!msg)
    {
        LOG_ERROR("alloc sms msg failed!");
        return;
    }

    msg->cmd = CMD_THREAD_SMS;
    msg->length = sizeof(SMS_SEND_INFO) + strlen(ack_message);

    data = (SMS_SEND_INFO*)msg->data;
    strncpy(data->number,number,TEL_NO_LENGTH + 1);
    data->type = SMS_SEND_DIRECT;
    data->smsLen = strlen(ack_message);
    strncpy(data->content,ack_message,ACK_MESSAGE_LEN);

    sendMsg(THREAD_MAIN, msg, msgLen);

    return;
}

static void eat_sms_flash_message_cb(EatSmsReadCnf_st smsFlashMessage)
{
    u8 format = 0;

    LOG_DEBUG("flash message.");
    eat_get_sms_format(&format);
    if(1 == format) //TEXTģʽ
    {
        LOG_DEBUG("recv TEXT sms.");
        LOG_DEBUG("msg=%s.",smsFlashMessage.data);
        LOG_DEBUG("datetime=%s.",smsFlashMessage.datetime);
        LOG_DEBUG("name=%s.",smsFlashMessage.name);
        LOG_DEBUG("status=%d.",smsFlashMessage.status);
        LOG_DEBUG("len=%d.",smsFlashMessage.len);
        LOG_DEBUG("number=%s.",smsFlashMessage.number);
    }
    else            //PDUģʽ
    {
        LOG_DEBUG("recv PDU sms.");
        LOG_DEBUG("msg=%s",smsFlashMessage.data);
        LOG_DEBUG("len=%d",smsFlashMessage.len);
    }
}

static void eat_sms_read_cb(EatSmsReadCnf_st smsReadCnfContent)
{
    u8 format = 0;
    unsigned char *p = smsReadCnfContent.data;

    LOG_DEBUG("new message.");
    eat_get_sms_format(&format);
    if(1 == format) //TEXTģʽ
    {
        LOG_DEBUG("recv TEXT sms.");

        sms_version_proc(p, smsReadCnfContent.number);
        sms_server_proc(p, smsReadCnfContent.number);
        sms_reboot_proc(p, smsReadCnfContent.number);
        sms_factory_proc(p, smsReadCnfContent.number);
    }
    else            //PDUģʽ
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

