/*
 * fsm.c
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#include <stdio.h>

#include <eat_timer.h>

#include "fsm.h"
#include "log.h"
#include "timer.h"
#include "setting.h"
#include "error.h"
#include "modem.h"
#include "request.h"


static STATUS status = STS_INITIAL;

static void fsm_trans(STATUS sts)
{
    status = sts;
}

static void start_mainloop(void)
{
    eat_timer_start(TIMER_AT_CMD, setting.at_cmd_timer_period);
}

typedef int ACTION(void);

int action_CallReady(void)
{
    modem_ReadGPRSStatus();

    fsm_trans(STS_WAIT_GPRS);

    start_mainloop();

    return 0;
}

//这个状态的事件处理最复杂，因为里面的API既有可能是同步的，也有可能是异步的，造成这一状态可以向另外四个状态迁移
int action_GprsAttached(void)
{
    int rc = socket_init();

    switch (rc)
    {
    case SUCCESS:
        fsm_trans(STS_WAIT_BEARER);
        break;

    case ERR_SOCKET_WAITING:
        fsm_trans(STS_WAIT_SOCKET);
        break;

    case ERR_SOCKET_CONNECTED:
        fsm_trans(STS_WAIT_LOGIN);
        break;

    case ERR_WAITING_HOSTNAME2IP:
        fsm_trans(STS_WAIT_IPADDR);
        break;

    case ERR_SOCKET_CONNECTED:
        fsm_trans(STS_WAIT_LOGIN);
        break;

    default:
        //状态不变，继续attach GPRS
        break;

    }

    return 0;

}


int action_BearHold(void)
{
    int rc = socket_setup();

    switch (rc)
    {
    case ERR_SOCKET_WAITING:
        fsm_trans(STS_WAIT_SOCKET);
        break;

    case ERR_WAITING_HOSTNAME2IP:
        fsm_trans(STS_WAIT_IPADDR);
        break;

    case ERR_SOCKET_CONNECTED:
        cmd_Login();
        fsm_trans(STS_WAIT_LOGIN);

        break;
    default:
        break;
    }

    return 0;
}

int action_SocketConnected(void)
{
    cmd_Login();

    fsm_trans(STS_WAIT_LOGIN);

    return 0;
}

int action_logined(void)
{
    fsm_trans(STS_RUNNING);

    return 0;
}

int action_loop(void)
{
#define MAX_SOCKET_RETRY_TIMES  5
    static int socket_retry_times = 0;

    switch (status)
    {
    case STS_WAIT_GPRS:
        modem_ReadGPRSStatus();
        break;

    case STS_WAIT_SOCKET:
        if (socket_retry_times++ < MAX_SOCKET_RETRY_TIMES)
        {
            socket_connect();
        }
        else
        {
            socket_retry_times = 0;
            fsm_trans(STS_WAIT_GPRS);
        }
        break;

    default:
        //TODO:xxx
        break;
    }

    return 0;
}

ACTION* state_transitions[STS_MAX][EVT_MAX] =
{
                     /* EVT_LOOP        EVT_CALL_READY      EVT_GPRS_ATTACHED       EVT_BEARER_HOLD    EVT_HOSTNAME2IP      EVT_SOCKET_CONNECTED    EVT_LOGINED  EVT_HEARTBEAT_LOSE  EVT_SOCKET_DISCONNECTED */
/* STS_INITIAL      */  {NULL,          action_CallReady,   NULL,           NULL,           NULL,       NULL,       NULL},
/* STS_WAIT_GPRS    */  {action_loop,   NULL,               action_GprsAttached,},
/* STS_WAIT_BEARER  */  {NULL,          NULL,               NULL,                   action_BearHold,},
/* STS_WAIT_SOCKET  */  {NULL,          NULL,               NULL,                   NULL,               NULL,               action_SocketConnected,},
/* STS_WAIT_IPADDR  */  {NULL,},
/* STS_WAIT_LOGIN   */  {NULL,          NULL,               NULL,                   NULL,               NULL,       NULL,   NULL,                   action_logined},
/* STS_RUNNING      */   {NULL,},
};

int fsm_run(EVENT event)
{
    ACTION* action = state_transitions[status][event];

    LOG_DEBUG("run FSM State(%d), Event(%d)", status, event);
    if (action)
    {
        return action();
    }
    return 0;
}

