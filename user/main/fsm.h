/*
 * fsm.h
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#ifndef USER_MAIN_FSM_H_
#define USER_MAIN_FSM_H_


typedef enum
{
    STS_INITIAL,
    STS_WAIT_GPRS,
    STS_WAIT_BEARER,
    STS_WAIT_SOCKET,
    STS_WAIT_IPADDR,
    STS_WAIT_LOGIN,
    STS_RUNNING,
    STS_MAX
}STATUS;

typedef enum
{
    EVT_LOOP,
    EVT_CALL_READY,
    EVT_GPRS_ATTACHED,
    EVT_BEARER_HOLD,
    EVT_HOSTNAME2IP,
    EVT_SOCKET_CONNECTED,
    EVT_LOGINED,
    EVT_HEARTBEAT_LOSE,
    EVT_SOCKET_DISCONNECTED,
    EVT_MAX
}EVENT;

int fsm_run(EVENT);

#endif /* USER_MAIN_FSM_H_ */
