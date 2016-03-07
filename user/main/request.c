/*
 * request.c
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#include <string.h>

#include <eat_interface.h>

#include "request.h"
#include "setting.h"
#include "msg.h"
#include "log.h"
#include "socket.h"
#include "version.h"

int cmd_Login(void)
{
    MSG_LOGIN_REQ* msg = alloc_msg(CMD_LOGIN, sizeof(MSG_LOGIN_REQ));
    u8 imei[IMEI_LENGTH] = {0};

    if (!msg)
    {
        LOG_ERROR("alloc login message failed!");
        return -1;
    }

    msg->version = htonl(VERSION_NUM);

    eat_get_imei(imei, IMEI_LENGTH);

    memcpy(msg->IMEI, imei, IMEI_LENGTH);

    LOG_DEBUG("send login message.");
    socket_sendData(msg, sizeof(MSG_LOGIN_REQ));

    return 0;
}

void cmd_Heartbeat(void)
{
    u8 msgLen = sizeof(MSG_HEADER) + sizeof(short);
    MSG_PING_REQ* msg = alloc_msg(CMD_PING, msgLen);
    msg->status = htons(EAT_TRUE);   //TODO: to define the status bits

    socket_sendData(msg, msgLen);
}

int cmd_SMS(const void* msg)
{
    return 0;
}


void cmd_Wild(const void* m, int len)
{
    u8 msgLen = sizeof(MSG_HEADER) + len;
    MSG_HEADER* msg = alloc_msg(CMD_WILD, msgLen);

    memcpy(msg + 1, m, len);

    socket_sendData(msg, msgLen);
}

int cmd_seek(unsigned int value)
{
    MSG_433* seek_msg = alloc_msg(CMD_433, sizeof(MSG_433));
    if (!seek_msg)
    {
        LOG_ERROR("alloc seek message failed!");
        return -1;
    }

    LOG_DEBUG("send seek value message:%d", value);
    seek_msg->intensity = htonl(value);
    socket_sendData(seek_msg, sizeof(MSG_433));

    return 0;
}

