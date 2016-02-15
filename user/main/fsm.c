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
#include "socket.h"


#define DESC_DEF(x) case x:\
                        return #x

static STATE status = STATE_INITIAL;

static char* fsm_getStateName(STATE sts)
{
    switch (sts)
    {
#ifdef LOG_DEBUG_FLAG
        DESC_DEF(STATE_INITIAL);
        DESC_DEF(STATE_WAIT_GPRS);
        DESC_DEF(STATE_WAIT_BEARER);
        DESC_DEF(STATE_WAIT_SOCKET);
        DESC_DEF(STATE_WAIT_IPADDR);
        DESC_DEF(STATE_WAIT_LOGIN);
        DESC_DEF(STATE_RUNNING);
#endif
        default:
        {
            static char state_name[10] = {0};
            sprintf(state_name, "%d", status);
            return state_name;
        }
    }
}

static char* fsm_getEventName(EVENT event)
{
    switch (event)
    {
#ifdef LOG_DEBUG_FLAG
        DESC_DEF(EVT_LOOP);
        DESC_DEF(EVT_CALL_READY);
        DESC_DEF(EVT_GPRS_ATTACHED);
        DESC_DEF(EVT_BEARER_HOLD);
        DESC_DEF(EVT_HOSTNAME2IP);
        DESC_DEF(EVT_SOCKET_CONNECTED);
        DESC_DEF(EVT_LOGINED);
        DESC_DEF(EVT_HEARTBEAT_LOSE);
        DESC_DEF(EVT_SOCKET_DISCONNECTED);
#endif
        default:
        {
            static char event_name[10] = {0};
            sprintf(event_name, "%d", status);
            return event_name;
        }
    }
}

static void fsm_trans(STATE sts)
{
    LOG_DEBUG("state: %s -> %s", fsm_getStateName(status), fsm_getStateName(sts));

    status = sts;
}

static void start_mainloop(void)
{
    eat_timer_start(TIMER_LOOP, setting.at_cmd_timer_period);
}

typedef int ACTION(void);

int action_CallReady(void)
{
    modem_ReadGPRSStatus();

    fsm_trans(STATE_WAIT_GPRS);

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
        fsm_trans(STATE_WAIT_BEARER);
        break;

    case ERR_SOCKET_WAITING:
        fsm_trans(STATE_WAIT_SOCKET);
        break;

    case ERR_SOCKET_CONNECTED:
        fsm_trans(STATE_WAIT_LOGIN);
        break;

    case ERR_WAITING_HOSTNAME2IP:
        fsm_trans(STATE_WAIT_IPADDR);
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
        fsm_trans(STATE_WAIT_SOCKET);
        break;

    case ERR_WAITING_HOSTNAME2IP:
        fsm_trans(STATE_WAIT_IPADDR);
        break;

    case ERR_SOCKET_CONNECTED:
        cmd_Login();
        fsm_trans(STATE_WAIT_LOGIN);

        break;
    default:
        break;
    }

    return 0;
}

int action_SocketConnected(void)
{
    cmd_Login();

    fsm_trans(STATE_WAIT_LOGIN);

    return 0;
}

int action_logined(void)
{
    fsm_trans(STATE_RUNNING);

    return 0;
}

int action_hostname2ip(void)
{
    fsm_trans(STATE_WAIT_SOCKET);

    return 0;
}

int action_reconnect(void)
{
    fsm_trans(STATE_WAIT_SOCKET);

    return 0;
}

int action_loop(void)
{
#define MAX_SOCKET_RETRY_TIMES  5
    static int socket_retry_times = 0;

    switch (status)
    {
    case STATE_WAIT_GPRS:
        modem_ReadGPRSStatus();
        break;

    case STATE_WAIT_SOCKET:
        //TODO: maybe the retry of connect is unnecessary here
        if (socket_retry_times++ < MAX_SOCKET_RETRY_TIMES)
        {
            if (socket_connect() == ERR_WAITING_HOSTNAME2IP)
            {
                fsm_trans(STATE_WAIT_IPADDR);
            }
        }
        else
        {
            socket_retry_times = 0;
            fsm_trans(STATE_WAIT_GPRS);
        }
        break;

    default:
        //TODO:xxx
        break;
    }

    return 0;
}

ACTION* state_transitions[STATE_MAX][EVT_MAX] =
{
                     /* EVT_LOOP        EVT_CALL_READY      EVT_GPRS_ATTACHED       EVT_BEARER_HOLD     EVT_HOSTNAME2IP     EVT_SOCKET_CONNECTED    EVT_LOGINED     EVT_HEARTBEAT_LOSE  EVT_SOCKET_DISCONNECTED */
/* STS_INITIAL      */  {NULL,          action_CallReady, },
/* STS_WAIT_GPRS    */  {action_loop,   NULL,               action_GprsAttached,},
/* STS_WAIT_BEARER  */  {NULL,          NULL,               NULL,                   action_BearHold,},
/* STS_WAIT_SOCKET  */  {action_loop,   NULL,               NULL,                   NULL,               NULL,               action_SocketConnected,},
/* STS_WAIT_IPADDR  */  {NULL,          NULL,               NULL,                   NULL,               action_hostname2ip,},
/* STS_WAIT_LOGIN   */  {NULL,          NULL,               NULL,                   NULL,               NULL,               NULL,                   action_logined, NULL,               action_reconnect},
/* STS_RUNNING      */  {NULL,          NULL,               NULL,                   NULL,               NULL,               NULL,                   NULL,           NULL,               action_reconnect},
};

//根据当前状态和出发事件，查找状态转换表，决定执行动作，在每个动作里面决定状态迁移
int fsm_run(EVENT event)
{
    ACTION* action = state_transitions[status][event];

    LOG_DEBUG("run FSM State(%s), Event(%s)", fsm_getStateName(status), fsm_getEventName(event));
    if (action)
    {
        return action();
    }
    return 0;
}

