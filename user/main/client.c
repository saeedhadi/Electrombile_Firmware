/*
 * client.c
 *
 *  Created on: 2015/7/9
 *      Author: jk
 */

#include <stdio.h>
#include <string.h>


#include <eat_interface.h>

#include "client.h"
#include "socket.h"
#include "mileage.h"
#include  "msg.h"
#include "log.h"
#include "uart.h"
#include "data.h"
#include "setting.h"
#include "thread.h"
#include "thread_msg.h"
#include "timer.h"
#include "seek.h"

#include "response.h"





typedef int (*MSG_PROC)(const void* msg);
typedef struct
{
    char cmd;
    MSG_PROC pfn;
}MC_MSG_PROC;


static MC_MSG_PROC msgProcs[] =
{
    {CMD_LOGIN, cmd_Login_rsp},
    {CMD_PING,  cmd_Ping_rsp},
    {CMD_ALARM, cmd_Alarm_rsp},
    {CMD_SMS,   cmd_Sms_rsp},
	{CMD_DEFEND, cmd_Defend_rsp},
    {CMD_SEEK,  cmd_Seek_rsp},
	{CMD_LOCATION, cmd_Location_rsp},
	{CMD_AUTODEFEND_SWITCH_SET, cmd_AutodefendSwitchSet_rsp},
    {CMD_AUTODEFEND_SWITCH_GET, cmd_AutodefendSwitchGet_rsp},
    {CMD_AUTODEFEND_PERIOD_SET, cmd_AutodefendPeriodSet_rsp},
    {CMD_AUTODEFEND_PERIOD_GET, cmd_AutodefendPeriodGet_rsp},
    {CMD_SERVER, cmd_Server_rsp},
    {CMD_TIMER, cmd_Timer_rsp},
    {CMD_BATTERY, cmd_Battery_rsp},
};

int client_proc(const void* m, int msgLen)
{
    MSG_HEADER* msg = (MSG_HEADER*)m;
    size_t i = 0;

    LOG_HEX(m, msgLen);

    if (msgLen < sizeof(MSG_HEADER))
    {
        LOG_ERROR("receive message length not enough: %zu(at least(%zu)!", msgLen, sizeof(MSG_HEADER));
        return -1;
    }

    if (msg->signature != ntohs(START_FLAG))
    {
        LOG_ERROR("receive message head signature error:%d!", msg->signature);
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
                LOG_ERROR("Message %d not processed!", msg->cmd);
                return -1;
            }
        }
    }

    LOG_ERROR("unknown message %d!", msg->cmd);
    return -1;
}

