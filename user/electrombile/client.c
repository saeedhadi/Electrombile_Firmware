/*
 * client.c
 *
 *  Created on: 2015/7/9
 *      Author: jk
 */

#include <stdio.h>
#include <string.h>
#include <time.h>


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
#include "timer.h"
#include "seek.h"





typedef int (*MSG_PROC)(const void* msg);
typedef struct
{
    char cmd;
    MSG_PROC pfn;
}MC_MSG_PROC;

extern double mileage;
static eat_bool mileage_flag = EAT_FALSE;


static int login_rsp(const void* msg);
static int ping_rsp(const void* msg);
static int alarm_rsp(const void* msg);
static int sms(const void* msg);
static int defend(const void* msg);
static int seek(const void* msg);
static int location(const void* msgLocation);
static int autodefend_switch_set(const void* msg);
static int autodefend_switch_get(const void* msg);
static int autodefend_period_set(const void* msg);
static int autodefend_period_get(const void* msg);
static time_t timestamp_get(void);
static int server_proc(const void* msg);
static int GPS_time_proc(const void* msg);
static void msg_mileage_send(MSG_MILEAGE_REQ msg_mileage);



static MC_MSG_PROC msgProcs[] =
{
    {CMD_LOGIN, login_rsp},
    {CMD_PING,  ping_rsp},
    {CMD_ALARM, alarm_rsp},
    {CMD_SMS,   sms},
	{CMD_DEFEND, defend},
    {CMD_SEEK,  seek},
	{CMD_LOCATION, location},
	{CMD_AUTODEFEND_SWITCH_SET, autodefend_switch_set},
    {CMD_AUTODEFEND_SWITCH_GET, autodefend_switch_get},
    {CMD_AUTODEFEND_PERIOD_SET, autodefend_period_set},
    {CMD_AUTODEFEND_PERIOD_GET, autodefend_period_get},
    {CMD_SERVER, server_proc},
    {CMD_TIMER, GPS_time_proc},
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

void client_loop(void)
{
    if (socket_conneted())
    {
        if (!client_logined())
        {
            MSG_LOGIN_REQ* msg = alloc_msg(CMD_LOGIN, sizeof(MSG_LOGIN_REQ));
            u8 imei[IMEI_LENGTH] = {0};

            if (!msg)
            {
                LOG_ERROR("alloc login message failed!");
                return;
            }

            eat_get_imei(imei, IMEI_LENGTH);
            imei[IMEI_LENGTH-1] = '0';

            memcpy(msg->IMEI, imei, IMEI_LENGTH);

            LOG_DEBUG("send login message.");
            socket_sendData(msg, sizeof(MSG_LOGIN_REQ));
        }

    }

}

static int login_rsp(const void* msg)
{
    LOG_DEBUG("get login respond.");

    set_client_state(EAT_TRUE);

    return 0;
}

static int ping_rsp(const void* msg)
{
    LOG_DEBUG("get ping respond.");

    return 0;
}

static int alarm_rsp(const void* msg)
{
    MSG_ALARM_REQ* req = (MSG_ALARM_REQ*)msg;
    //MSG_ALARM_RSP* rsp = NULL;

    switch(req->alarmType)
    {
        case ALARM_VIBRATE:
            LOG_DEBUG("get alarm(ALARM_VIBRATE) respond.");
            break;

        case ALARM_FENCE_OUT:
            LOG_DEBUG("get alarm(ALARM_FENCE_OUT) respond.");
            break;

        case ALARM_FENCE_IN:
            LOG_DEBUG("get alarm(ALARM_FENCE_IN) respond.");
            break;

        default:
            break;
    }

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
            mileagehandle(EAT_TRUE,EAT_FALSE);
    		break;

    	case DEFEND_GET:
    		result = vibration_fixed() ? DEFEND_ON : DEFEND_OFF;
            LOG_DEBUG("get defend switch state(%d).", result);
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
		setSeekMode(EAT_TRUE);
        eat_timer_start(TIMER_SEEKAUTOOFF,setting.seekautooff_timer_peroid);

        LOG_DEBUG("seek auto_off is on ,time is %ds",setting.seekautooff_timer_peroid/1000);

        LOG_DEBUG("set seek on.");
	}
	else if(req->operator == SEEK_OFF)
	{
		setSeekMode(EAT_FALSE);
        LOG_DEBUG("set seek off.");
	}

	rsp = alloc_rspMsg(&req->header);
	if (!rsp)
	{
		LOG_ERROR("alloc seek rsp message failed!");
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

static int autodefend_switch_set(const void* msg)
{
    MSG_AUTODEFEND_SWITCH_SET_REQ* req = (MSG_AUTODEFEND_SWITCH_SET_REQ*)msg;
	MSG_AUTODEFEND_SWITCH_SET_RSP* rsp = NULL;

	if(req->onOff == AUTO_DEFEND_ON)
	{
		set_autodefend_state(EAT_TRUE);
        LOG_DEBUG("set autodefend swtich on.");
	}
	else if(req->onOff == AUTO_DEFEND_OFF)
	{
		set_autodefend_state(EAT_FALSE);
        LOG_DEBUG("set autodefend swtich off.");
	}

	rsp = alloc_rspMsg(&req->header);
	if (!rsp)
	{
		LOG_ERROR("alloc autodefend_swtich_set rsp message failed!");
		return -1;
	}

	rsp->token = req->token;
	rsp->result = MSG_SUCCESS;

    socket_sendData(rsp, sizeof(MSG_AUTODEFEND_SWITCH_SET_RSP));

    return 0;
}

static int autodefend_switch_get(const void* msg)
{
    MSG_AUTODEFEND_SWITCH_GET_REQ* req = (MSG_AUTODEFEND_SWITCH_GET_REQ*)msg;
	MSG_AUTODEFEND_SWITCH_GET_RSP* rsp = NULL;

    rsp = alloc_rspMsg(&req->header);
	if (!rsp)
	{
		LOG_ERROR("alloc autodefend_swtich_get rsp message failed!");
		return -1;
	}

	rsp->token = req->token;
	rsp->result = get_autodefend_state() ? AUTO_DEFEND_ON : AUTO_DEFEND_OFF;

    socket_sendData(rsp, sizeof(MSG_AUTODEFEND_SWITCH_GET_RSP));

    return 0;
}

static int autodefend_period_set(const void* msg)
{
    MSG_AUTODEFEND_PERIOD_SET_REQ* req = (MSG_AUTODEFEND_PERIOD_SET_REQ*)msg;
	MSG_AUTODEFEND_PERIOD_SET_RSP* rsp = NULL;

    LOG_DEBUG("set autodefend period as %dmins.", req->period);
    set_autodefend_period(req->period);

    rsp = alloc_rspMsg(&req->header);
	if (!rsp)
	{
		LOG_ERROR("alloc autodefend_period_set rsp message failed!");
		return -1;
	}

	rsp->token = req->token;
    rsp->result = MSG_SUCCESS;

    socket_sendData(rsp, sizeof(MSG_AUTODEFEND_PERIOD_SET_RSP));

    return 0;
}

static int autodefend_period_get(const void* msg)
{
    MSG_AUTODEFEND_PERIOD_GET_REQ* req = (MSG_AUTODEFEND_PERIOD_GET_REQ*)msg;
	MSG_AUTODEFEND_PERIOD_GET_RSP* rsp = NULL;

    rsp = alloc_rspMsg(&req->header);
	if (!rsp)
	{
		LOG_ERROR("alloc autodefend_period_get rsp message failed!");
		return -1;
	}

	rsp->token = req->token;
    rsp->period = get_autodefend_period();
    LOG_INFO("alloc autodefend_period_get rsp message as %dmins",rsp->period);
    socket_sendData(rsp, sizeof(MSG_AUTODEFEND_PERIOD_GET_RSP));

    return 0;
}

static int GPS_time_proc(const void* msg)
{
    MSG_GPSTIMER_REQ* req = (MSG_GPSTIMER_REQ*)msg;
    MSG_GPSTIMER_RSP* rsp = NULL;
    if(0 >= req->timer)
    {
        ;//rsp at the end
    }
    else if(10 >= req->timer)
    {
        setting.gps_send_timer_period = 10 * 1000;
        convert_setting_to_storage();
        storage_save();

        eat_timer_stop(TIMER_GPS_SEND);
        eat_timer_start(TIMER_GPS_SEND, setting.gps_send_timer_period);
        LOG_INFO("SET TIMER to %d OK!",setting.gps_send_timer_period);
    }
    else if(21600 <= req->timer)
    {
        setting.gps_send_timer_period = 21600 * 1000;
        convert_setting_to_storage();
        storage_save();

        eat_timer_stop(TIMER_GPS_SEND);
        eat_timer_start(TIMER_GPS_SEND, setting.gps_send_timer_period);

        LOG_INFO("SET TIMER to %d OK!",setting.gps_send_timer_period);
    }
    else if((10 < req->timer)&&(21600 > req->timer))
    {
        setting.gps_send_timer_period = req->timer * 1000;
        convert_setting_to_storage();
        storage_save();

        eat_timer_stop(TIMER_GPS_SEND);
        eat_timer_start(TIMER_GPS_SEND, setting.gps_send_timer_period);

        LOG_INFO("SET TIMER to %d OK", setting.gps_send_timer_period);
    }
    rsp = alloc_rspMsg(&req->header);

    rsp->result = setting.gps_send_timer_period;
    socket_sendData(rsp,sizeof(MSG_GPSTIMER_RSP));

    return 0;
}
static int server_proc(const void* msg)
{
    MSG_SERVER* msg_server = (MSG_SERVER*)msg;
    u32 ip[4] = {0};
    int count;
    signed char domain[MAX_DOMAIN_NAME_LEN] = {0};

    count = sscanf(msg_server->server,"%u.%u.%u.%u",&ip[0],&ip[1],&ip[2],&ip[3]);
    if(4 == count)
    {
        setting.addr_type = ADDR_TYPE_IP;
        setting.ipaddr[0] = (u8)ip[0];
        setting.ipaddr[1] = (u8)ip[1];
        setting.ipaddr[2] = (u8)ip[2];
        setting.ipaddr[3] = (u8)ip[3];
        setting.port = (u16)msg_server->port;
        convert_setting_to_storage();
        storage_save();
        LOG_INFO("server proc %s:%d successful!",msg_server->server,msg_server->port);

        eat_reset_module();
    }
    else
    {
        count = sscanf(msg_server->server, "%[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.]", domain);
        if(1 == count)
        {
            setting.addr_type = ADDR_TYPE_DOMAIN;
            strcpy(setting.domain, msg_server->server);
            setting.port = (u16)msg_server->port;
            convert_setting_to_storage();
            storage_save();
            LOG_INFO("server proc %s:%d successful!",msg_server->server,msg_server->port);

            eat_reset_module();
        }
        else
        {
            LOG_DEBUG("server proc %s:%d error!",msg_server->server,msg_server->port);
        }
    }

    return 0;
}



void msg_wild(const void* m, int len)
{
    u8 msgLen = sizeof(MSG_HEADER) + len;
    MSG_HEADER* msg = alloc_msg(CMD_WILD, msgLen);

    memcpy(msg + 1, m, len);

    socket_sendData(msg, msgLen);
}
void mileagehandle(eat_bool start_flag,eat_bool end_flag)
{
    static MSG_MILEAGE_REQ msg_mileage;
    time_t timestamp;
    timestamp = timestamp_get();
    LOG_DEBUG("timestamp:%ld",timestamp);
    if(EAT_TRUE == start_flag && EAT_FALSE == end_flag)
    {
        msg_mileage.starttime = timestamp;
        msg_mileage.mileage = 0;
        mileage_flag = EAT_TRUE;
        mileage = 0.f;
    }
    else if(EAT_TRUE == end_flag && EAT_FALSE == start_flag && mileage_flag == EAT_TRUE)
    {
        msg_mileage.endtime = timestamp;
        msg_mileage.mileage = (int)mileage;
        msg_mileage_send(msg_mileage);
        LOG_DEBUG("send this mileage :%d m",msg_mileage.mileage);
        mileage_flag = EAT_FALSE;
    }
    else
    {
        mileage_flag = EAT_FALSE;
        return;
    }
}

static void msg_mileage_send(MSG_MILEAGE_REQ msg_mileage)
{
    u8 msgLen = sizeof(MSG_MILEAGE_REQ);
    MSG_MILEAGE_REQ* msg = alloc_msg(CMD_MILEAGE, msgLen);
    msg->endtime = msg_mileage.endtime;
    msg->starttime = msg_mileage.starttime;
    msg->mileage = (int)msg_mileage.mileage;
    LOG_INFO("send the mileage");
    socket_sendData(msg, msgLen);
}

static time_t timestamp_get(void)
{
    struct tm stm = {0};
    EatRtc_st rtc = {0};

    eat_get_rtc(&rtc);

    stm.tm_year = rtc.year + YEAROFFSET - 1900;
    stm.tm_mon = rtc.mon - 1;
    stm.tm_mday = rtc.day;
    stm.tm_hour = rtc.hour;
    stm.tm_min = rtc.min;
    stm.tm_sec = rtc.sec;

    return mktime(&stm);
}

void send_autodefendstate_msg(eat_bool state)
{
    u8 msgLen = sizeof(MSG_HEADER) + sizeof(char);
	MSG_AUTODEFEND_STATE_REQ* msg = alloc_msg(CMD_AUTODEFEND_STATE, msgLen);
    msg->state = state;   //TODO: to send the state of the autodefend

	socket_sendData(msg, msgLen);

}

void msg_heartbeat(void)
{
    u8 msgLen = sizeof(MSG_HEADER) + sizeof(short);
    MSG_PING_REQ* msg = alloc_msg(CMD_PING, msgLen);
    msg->statue = EAT_TRUE;   //TODO: to define the status bits

    socket_sendData(msg, msgLen);
}

