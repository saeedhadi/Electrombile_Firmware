/*
 * client.c
 *
 *  Created on: 2015��7��9��
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


static MC_MSG_PROC msgProcs[] =
{
    {CMD_LOGIN, login_rsp},
    {CMD_PING,  ping_rsp},
    {CMD_ALARM, alarm_rsp},
    {CMD_SMS,   sms},
	{CMD_DEFEND, defend},
    {CMD_SEEK,  seek},
};


static void print_hex(const char* data, int length)
{
    int i = 0, j = 0;

    print("    ");
    for (i  = 0; i < 16; i++)
    {
        print("%X  ", i);
    }
    print("    ");
    for (i = 0; i < 16; i++)
    {
        print("%X", i);
    }

    print("\r\n");

    for (i = 0; i < length; i += 16)
    {
        print("%02d  ", i / 16 + 1);
        for (j = i; j < i + 16 && j < length; j++)
        {
            print("%02x ", data[j] & 0xff);
        }
        if (j == length && length % 16)
        {
            for (j = 0; j < (16 - length % 16); j++)
            {
                print("   ");
            }
        }
        print("    ");
        for (j = i; j < i + 16 && j < length; j++)
        {
            if (data[j] < 32)
            {
                print(".");
            }
            else
            {
                print("%c", data[j] & 0xff);
            }
        }

        print("\r\n");
    }
}

int client_proc(const void* m, int msgLen)
{
    MSG_HEADER* msg = (MSG_HEADER*)m;
    size_t i = 0;

    print_hex(m, msgLen);

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
                print_hex((const char*)msg, sizeof(MSG_GPS));
                socket_sendData(msg, sizeof(MSG_GPS));
            }
            else
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
                print_hex((const char*)msg, msgLen);
                socket_sendData(msg, msgLen);
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
            print_hex((const char*)msg, sizeof(MSG_LOGIN_REQ));
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
		LOG_ERROR("unknown operator %d", req->operator);
//		break;
		return 0;
	}


	rsp = alloc_rspMsg(&req->header);
	if (!rsp)
	{
		LOG_ERROR("alloc defend rsp message failed");
		return -1;
	}

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

	rsp->result = MSG_SUCCESS;

    socket_sendData(rsp, sizeof(MSG_SEEK_RSP));

    return 0;
}

