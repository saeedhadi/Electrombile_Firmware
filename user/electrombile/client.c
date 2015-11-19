/*
 * client.c
 *
 *  Created on: 2015Äê7ÔÂ9ÈÕ
 *      Author: jk
 */

#include <stdio.h>
#include <string.h>

#include <eat_interface.h>

#include "client.h"
#include "socket.h"
#include "msg.h"
#include "log.h"
#include "uart.h"
#include "data.h"
#include "setting.h"
#include "thread.h"
#include "thread_msg.h"


typedef int (*MSG_PROC)(const void* msg);
typedef struct
{
    char cmd;
    MSG_PROC pfn;
}MC_MSG_PROC;


static int login_rsp(const void* msg);
static int ping_rsp(const void* msg);
static int alarm_rsp(const void* msg);
static int sms(const void* msg);
static int defend(const void* msg);
static int seek(const void* msg);
static int location(const void* msgLocation);

static MC_MSG_PROC msgProcs[] =
{
    {CMD_LOGIN, login_rsp},
    {CMD_PING,  ping_rsp},
    {CMD_ALARM, alarm_rsp},
    {CMD_SMS,   sms},
	{CMD_DEFEND, defend},
    {CMD_SEEK,  seek},
	{CMD_LOCATION, location},
};

int client_proc(const void* m, int msgLen)
{
    MSG_HEADER* msg = (MSG_HEADER*)m;
    size_t i = 0;

    log_hex(m, msgLen);

    if (msgLen < sizeof(MSG_HEADER))
    {
        LOG_ERROR("receive message length not enough: %zu(at least(%zu)", msgLen, sizeof(MSG_HEADER));
        return -1;
    }

    if (msg->signature != ntohs(START_FLAG))
    {
        LOG_ERROR("receive message head signature error:%d", msg->signature);
        return -1;
    }

    for (i = 0; i < sizeof(msgProcs) / sizeof(msgProcs[0]); i++)
    {
        if (msgProcs[i].cmd == msg->cmd)
        {
            MSG_PROC pfn = msgProcs[i].pfn;
            if (pfn)
            {
                return pfn(msg);
            }
            else
            {
                LOG_ERROR("Message %d not processed", msg->cmd);
                return -1;
            }
        }
    }

    LOG_ERROR("unknown message %d", msg->cmd);
    return -1;
}

void client_loop(void)
{
    int i = 0;  //loop iterator

    if (socket_conneted())
    {
        if (client_logined())
        {
            if (data.isGpsFixed)
            {
                MSG_GPS* msg = alloc_msg(CMD_GPS, sizeof(MSG_GPS));
                if (!msg)
                {
                    LOG_ERROR("alloc message failed");
                    return;
                }

                msg->gps.longitude = data.gps.longitude;
                msg->gps.latitude = data.gps.latitude;

                LOG_DEBUG("send GPS message");
                log_hex((const char*)msg, sizeof(MSG_GPS));
                socket_sendData(msg, sizeof(MSG_GPS));

                data.isGpsFixed = EAT_FALSE;
            }
            else if(data.isCellGet)
            {
                size_t msgLen = sizeof(MSG_HEADER) + sizeof(CGI) + sizeof(CELL) * data.cgi.cellNo;
                MSG_HEADER* msg = alloc_msg(CMD_CELL, msgLen);
                CGI* cgi = (CGI*)(msg + 1);
                CELL* cell = (CELL*)(cgi + 1);
                if (!msg)
                {
                    LOG_ERROR("alloc message failed");
                    return;
                }

                cgi->mcc = htons(data.cgi.mcc);
                cgi->mnc = htons(data.cgi.mnc);
                cgi->cellNo = data.cgi.cellNo;
                for (i = 0; i < data.cgi.cellNo; i++)
                {
                    cell[i].lac = htons(data.cells[i].lac);
                    cell[i].cellid = htons(data.cells[i].cellid);
                    cell[i].rxl= htons(data.cells[i].rxl);
                }

                LOG_DEBUG("send CELL message");
                log_hex((const char*)msg, msgLen);
                socket_sendData(msg, msgLen);

                data.isCellGet = EAT_FALSE;
            }
        }
        else
        {
            MSG_LOGIN_REQ* msg = alloc_msg(CMD_LOGIN, sizeof(MSG_LOGIN_REQ));
            u8 imei[IMEI_LENGTH] = {0};

            if (!msg)
            {
                LOG_ERROR("alloc message failed");
                return;
            }

            eat_get_imei(imei, IMEI_LENGTH);
            imei[IMEI_LENGTH-1] = '0';

            memcpy(msg->IMEI, imei, IMEI_LENGTH);

            LOG_DEBUG("send login message.");
            log_hex((const char*)msg, sizeof(MSG_LOGIN_REQ));
            socket_sendData(msg, sizeof(MSG_LOGIN_REQ));
        }

    }

}

static int login_rsp(const void* msg)
{
    set_client_state(EAT_TRUE);

    return 0;
}

static int ping_rsp(const void* msg)
{
    return 0;
}

static int alarm_rsp(const void* msg)
{
    MSG_ALARM_REQ* req = (MSG_ALARM_REQ*)msg;
    MSG_ALARM_RSP* rsp = NULL;

    switch(req->alarmType)
    {
        case ALARM_VIBRATE:
            set_vibration_state(EAT_TRUE);
            LOG_DEBUG("set vibration alarm on.");
            break;

        case ALARM_FENCE_OUT:
            break;

        case ALARM_FENCE_IN:
            break;

        default:
            break;
    }

    rsp = alloc_rspMsg(&req->header);
	if (!rsp)
	{
		LOG_ERROR("alloc alarm rsp message failed");
		return -1;
	}

    socket_sendData(rsp, sizeof(MSG_ALARM_RSP));

    return 0;
}

static int sms(const void* msg)
{
    return 0;
}

static int defend(const void* msg)
{
	MSG_DEFEND_REQ* req = (MSG_DEFEND_REQ*)msg;
	MSG_DEFEND_RSP* rsp = NULL;
	unsigned char result = MSG_SUCCESS;

	switch (req->operator)
	{
	case DEFEND_ON:
		LOG_DEBUG("set defend switch on.");
		set_vibration_state(EAT_TRUE);
		break;

	case DEFEND_OFF:
		LOG_DEBUG("set defend switch off.");
		set_vibration_state(EAT_FALSE);
		break;

	case DEFEND_GET:
		result = vibration_fixed() ? DEFEND_ON : DEFEND_OFF;
		break;
	default:
		LOG_ERROR("unknown operator %d!", req->operator);
		return 0;
	}

	rsp = alloc_rspMsg(&req->header);
	if (!rsp)
	{
		LOG_ERROR("alloc defend rsp message failed!");
		return -1;
	}

	rsp->token = req->token;
	rsp->result = result;

    socket_sendData(rsp, sizeof(MSG_DEFEND_REQ));

    return 0;
}

static int seek(const void* msg)
{
	MSG_SEEK_REQ* req = (MSG_SEEK_REQ*)msg;
	MSG_SEEK_RSP* rsp = NULL;

	if (req->operator == SEEK_ON)
	{
		set_seek_state(EAT_TRUE);
        LOG_DEBUG("set seek on.");
	}
	else
	{
		set_seek_state(EAT_FALSE);
        LOG_DEBUG("set seek off.");
	}

	rsp = alloc_rspMsg(&req->header);
	if (!rsp)
	{
		LOG_ERROR("alloc seek rsp message failed");
		return -1;
	}

	rsp->token = req->token;
	rsp->result = MSG_SUCCESS;

    socket_sendData(rsp, sizeof(MSG_SEEK_RSP));

    return 0;
}

static int location(const void* msgLocation)
{
    u8 msgLen = sizeof(MSG_THREAD);
    MSG_THREAD* msg = allocMsg(msgLen);

    msg->cmd = CMD_THREAD_LOCATION;
    msg->length = 0;

    LOG_DEBUG("send CMD_THREAD_LOCATION to THREAD_GPS.");
	sendMsg(THREAD_MAIN, THREAD_GPS, msg, msgLen);

	return 0;
}

void msg_heartbeat(void)
{
    u8 msgLen = sizeof(MSG_HEADER) + sizeof(short);
	MSG_PING_REQ* msg = alloc_msg(CMD_PING, msgLen);
    msg->statue = 33;   //TODO: to define the status bits
    //LOG_DEBUG("%c %d %d %c %c",msg->header.cmd,msg->header.length,msg->header.seq,msg->header.signature,msg->statue);

	socket_sendData(msg, msgLen);
}



