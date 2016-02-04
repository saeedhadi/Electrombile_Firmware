/*
 * request.c
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#include <string.h>

#include <eat_interface.h>

#include "request.h"
#include "msg.h"
#include "log.h"
#include "socket.h"

int cmd_Login(void)
{
    MSG_LOGIN_REQ* msg = alloc_msg(CMD_LOGIN, sizeof(MSG_LOGIN_REQ));
    u8 imei[IMEI_LENGTH] = {0};

    if (!msg)
    {
        LOG_ERROR("alloc login message failed!");
        return -1;
    }

    eat_get_imei(imei, IMEI_LENGTH);
    imei[IMEI_LENGTH-1] = '0';

    memcpy(msg->IMEI, imei, IMEI_LENGTH);

    LOG_DEBUG("send login message.");
    socket_sendData(msg, sizeof(MSG_LOGIN_REQ));

    return 0;
}

void cmd_Heartbeat(void)
{
    u8 msgLen = sizeof(MSG_HEADER) + sizeof(short);
    MSG_PING_REQ* msg = alloc_msg(CMD_PING, msgLen);
    msg->statue = EAT_TRUE;   //TODO: to define the status bits

    socket_sendData(msg, msgLen);
}

void cmd_Autodefendstate(eat_bool state)
{
    u8 msgLen = sizeof(MSG_HEADER) + sizeof(char);
    MSG_AUTODEFEND_STATE_REQ* msg = alloc_msg(CMD_AUTODEFEND_STATE, msgLen);
    msg->state = state;   //TODO: to send the state of the autodefend

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

