/*
 * event.c
 *
 *  Created on: 2015/6/25
 *      Author: jk
 */
#include <string.h>

#include <eat_interface.h>
#include <eat_uart.h>

#include "timer.h"
#include "watchdog.h"
#include "thread_msg.h"
#include "log.h"
#include "uart.h"
#include "socket.h"
#include "setting.h"
#include "msg.h"
#include "data.h"
#include "client.h"
#include "tool.h"

typedef int (*EVENT_FUNC)(const EatEvent_st* event);
typedef struct
{
	EatEvent_enum event;
	EVENT_FUNC pfn;
}EVENT_PROC;

typedef int (*THREAD_MSG_FUNC)(const MSG_THREAD* msg);
typedef struct
{
    char cmd;
    THREAD_MSG_FUNC pfn;
}THREAD_MSG_PROC;




#define DESC_DEF(x) case x:\
                            return #x

static char* getEventDescription(EatEvent_enum event)
{
    switch (event)
    {
        #ifdef LOG_DEBUG_FLAG
                DESC_DEF(EAT_EVENT_TIMER);
                DESC_DEF(EAT_EVENT_KEY);
                DESC_DEF(EAT_EVENT_INT);
                DESC_DEF(EAT_EVENT_MDM_READY_RD);
                DESC_DEF(EAT_EVENT_MDM_READY_WR);
                DESC_DEF(EAT_EVENT_MDM_RI);
                DESC_DEF(EAT_EVENT_UART_READY_RD);
                DESC_DEF(EAT_EVENT_UART_READY_WR);
                DESC_DEF(EAT_EVENT_ADC);
                DESC_DEF(EAT_EVENT_UART_SEND_COMPLETE);
                DESC_DEF(EAT_EVENT_USER_MSG);
                DESC_DEF(EAT_EVENT_IME_KEY);
        #endif
        default:
        {
            static char soc_event[10] = {0};
            sprintf(soc_event, "%d", event);
            return soc_event;
        }
    }
}


static int event_mod_ready_rd(const EatEvent_st* event)
{
	u8 buf[256] = {0};
	u16 len = 0;
	u8* buf_ptr = NULL;

	len = eat_modem_read(buf, 256);
	if (!len)
	{
	    LOG_ERROR("modem received nothing.");
	    return -1;
	}
    LOG_DEBUG("modem recv: %s", buf);

	buf_ptr = (u8*) strstr((const char *) buf, "+CGATT: 1");
	if (buf_ptr != NULL)
	{
		socket_init();
	}

	return 0;
}

static int event_timer(const EatEvent_st* event)
{
    switch (event->data.timer.timer_id)
    {
        case TIMER_WATCHDOG:
            LOG_INFO("TIMER_WATCHDOG expire.");
            feedWatchdog();
            eat_timer_start(event->data.timer.timer_id, setting.watchdog_timer_period);
            break;

        case TIMER_AT_CMD:
            LOG_INFO("TIMER_AT_CMD expire.");
            tool_modem_write("AT+CGATT?\n");
            eat_timer_start(event->data.timer.timer_id, setting.at_cmd_timer_period);
            break;

        case TIMER_GPS_SEND:
            LOG_INFO("TIMER_GPS_SEND expire.");
            eat_timer_start(event->data.timer.timer_id, setting.gps_send_timer_period);
            client_loop();
            break;

        case TIMER_SOCKET:
            LOG_INFO("TIMER_SOCKET expire.");
            socket_init();
            break;

        case TIMER_HEARTBEAT:
            LOG_INFO("TIMER_HEARTBEAT expire!");
            msg_heartbeat();
            eat_timer_start(TIMER_HEARTBEAT, setting.heartbeat_timer_period);
            break;

        case TIMER_SEEKAUTOOFF:
            LOG_INFO("TIMER_SEEKAUTOOFF expire!");
            setSeekMode(EAT_FALSE);
            break;

        default:
            LOG_ERROR ("timer(%d) not processed!", event->data.timer.timer_id);
            break;
    }
    return 0;
}

static void sendGPS2Server(LOCAL_GPS* gps)
{

    if (gps->isGps)
    {
        MSG_GPS* msg = alloc_msg(CMD_GPS, sizeof(MSG_GPS));
        if (!msg)
        {
            LOG_ERROR("alloc GPS message failed!");
            return;
        }

        msg->gps.longitude = gps->gps.longitude;
        msg->gps.latitude = gps->gps.latitude;
        msg->gps.altitude = gps->gps.altitude;
        msg->gps.speed = gps->gps.speed;
        msg->gps.course = gps->gps.course;

        LOG_DEBUG("send GPS message.");

        socket_sendData(msg, sizeof(MSG_GPS));
    }
#if 0
    else
    {
        size_t msgLen = sizeof(MSG_HEADER) + sizeof(CGI) + sizeof(CELL) * gps->cellInfo.cellNo;
        MSG_HEADER* msg = alloc_msg(CMD_CELL, msgLen);
        CGI* cgi = (CGI*)(msg + 1);
        CELL* cell = (CELL*)(cgi + 1);
        int i = 0;
        if (!msg)
        {
            LOG_ERROR("alloc CELL message failed!");
            return;
        }

        cgi->mcc = htons(gps->cellInfo.mcc);
        cgi->mnc = htons(gps->cellInfo.mnc);
        cgi->cellNo = gps->cellInfo.cellNo;
        for (i = 0; i < gps->cellInfo.cellNo; i++)
        {
            cell[i].lac = htons(gps->cellInfo.cell[i].lac);
            cell[i].cellid = htons(gps->cellInfo.cell[i].cellid);
            cell[i].rxl= htons(gps->cellInfo.cell[i].rxl);
        }

        LOG_DEBUG("send CELL message.");
        socket_sendData(msg, msgLen);

        data.isCellGet = EAT_FALSE;
    }
#endif
}
static int threadCmd_GPS(const MSG_THREAD* msg)
{
    LOCAL_GPS* gps = (LOCAL_GPS*) msg->data;

     if (msg->length < sizeof(LOCAL_GPS)  || !gps)
     {
         LOG_ERROR("msg from THREAD_GPS error!");
         return -1;
     }

     //just forward it to the server
     sendGPS2Server(gps);

    return 0;
}

static int threadCmd_SMS(const MSG_THREAD* msg)
{
    LOG_DEBUG("receive thread command CMD_SMS.");

    return 0;
}

static int threadCmd_Vibrate(const MSG_THREAD* msg)
{
    unsigned char* alarm_type = (unsigned char*)msg->data;
    MSG_ALARM_REQ* socket_msg = 0;

    if (msg->length != sizeof(*alarm_type))
    {
        LOG_ERROR("msg length error: msgLen(%d)!", msg->length);
        return -1;
    }
    LOG_DEBUG("receive thread command CMD_VIBRATE: alarmType(%d).", *alarm_type);

    socket_msg = alloc_msg(CMD_ALARM, sizeof(MSG_ALARM_REQ));
    if (!socket_msg)
    {
        LOG_ERROR("alloc message failed!");
        return -1;
    }

    LOG_DEBUG("send alarm vibrate message.");
    socket_msg->alarmType = *alarm_type;
    socket_sendData(socket_msg, sizeof(MSG_ALARM_REQ));

    return 0;
}

static int threadCmd_Seek(const MSG_THREAD* msg)
{
    SEEK_INFO* seek = (SEEK_INFO*)msg->data;
    MSG_433* seek_msg;

    if (msg->length < sizeof(SEEK_INFO)  || !seek)
    {
        LOG_ERROR("msg from THREAD_SEEK error!");
        return -1;
    }

    LOG_DEBUG("receive thread command CMD_SEEK: value(%f).", seek->intensity);

    seek_msg = alloc_msg(CMD_433, sizeof(MSG_433));
    if (!seek_msg)
    {
        LOG_ERROR("alloc message failed!");
        return -1;
    }

    LOG_DEBUG("send seek value message.");
    seek_msg->intensity = htonl((int)seek->intensity);
    socket_sendData(seek_msg, sizeof(MSG_433));

    return 0;
}

static int threadCmd_Location(const MSG_THREAD* msg)
{
    LOCAL_GPS* gps = (LOCAL_GPS*) msg->data;

    if (msg->length < sizeof(LOCAL_GPS)  || !gps)
    {
        LOG_ERROR("msg from THREAD_GPS error!");
        return -1;
    }

    if (gps->isGps)             //update the local GPS data
    {
        MSG_GPSLOCATION_RSP* msg = alloc_msg(CMD_LOCATION, sizeof(MSG_GPSLOCATION_RSP));
        if (!msg)
        {
            LOG_ERROR("alloc message failed!");
            return -1;
        }
        msg->isGps= gps->isGps;
        msg->gps.longitude = gps->gps.longitude;
        msg->gps.latitude = gps->gps.latitude;
        msg->gps.altitude = gps->gps.altitude;
        msg->gps.speed = gps->gps.speed;
        msg->gps.course = gps->gps.course;

        LOG_DEBUG("send GPS_LOCATION message.");
        socket_sendData(msg, sizeof(MSG_GPSLOCATION_RSP));
    }
    else                //update local cell info
    {
        size_t msgLen = sizeof(MSG_CELLLOCATION_HEADER) + sizeof(CGI) + sizeof(CELL) * gps->cellInfo.cellNo;
        MSG_CELLLOCATION_HEADER* msg = alloc_msg(CMD_LOCATION, msgLen);
        CGI* cgi = (CGI*)(msg + 1);
        CELL* cell = (CELL*)(cgi + 1);
        int i = 0;

        if (!msg)
        {
            LOG_ERROR("alloc message failed!");
            return -1;
        }
        msg->isGps = gps->isGps;

        cgi->mcc = htons(gps->cellInfo.mcc);
        cgi->mnc = htons(gps->cellInfo.mnc);
        cgi->cellNo = gps->cellInfo.cellNo;
        for (i = 0; i < gps->cellInfo.cellNo; i++)
        {
            cell[i].lac = htons(gps->cellInfo.cell[i].lac);
            cell[i].cellid = htons(gps->cellInfo.cell[i].cellid);
            cell[i].rxl= htons(gps->cellInfo.cell[i].rxl);
        }

        LOG_DEBUG("send CELL_LOCATION message.");
        socket_sendData(msg, msgLen);
    }

    return 0;
}


static THREAD_MSG_PROC msgProcs[] =
{
        {CMD_THREAD_GPS, threadCmd_GPS},
        {CMD_THREAD_SMS, threadCmd_SMS},
        {CMD_THREAD_VIBRATE, threadCmd_Vibrate},
        {CMD_THREAD_SEEK, threadCmd_Seek},
        {CMD_THREAD_LOCATION, threadCmd_Location},
};

static int event_threadMsg(const EatEvent_st* event)
{
    MSG_THREAD* msg = (MSG_THREAD*) event->data.user_msg.data_p;
    u8 msgLen = event->data.user_msg.len;
    size_t i = 0;
    int rc = 0;

    //check the message length
    if (msg->length + sizeof(MSG_THREAD) != msgLen)
    {
        LOG_ERROR("Message length error");
        freeMsg(msg);

        return -1;
    }

    for (i = 0; i < sizeof(msgProcs) / sizeof(msgProcs[0]); i++)
    {
        if (msgProcs[i].cmd == msg->cmd)
        {
            THREAD_MSG_FUNC pfn = msgProcs[i].pfn;
            if (pfn)
            {
                rc = pfn(msg);
                break;
            }
            else
            {
                LOG_ERROR("thread message %d not processed!", msg->cmd);
                rc = -1;
                break;
            }
        }
    }

    freeMsg(msg);

    return rc;
}




static EVENT_PROC eventProcs[] =
{
    {EAT_EVENT_TIMER,               event_timer},
    {EAT_EVENT_MDM_READY_RD,        event_mod_ready_rd},
    {EAT_EVENT_MDM_READY_WR,        EAT_NULL},
    {EAT_EVENT_UART_READY_RD,       event_uart_ready_rd},
    {EAT_EVENT_UART_READY_WR,       EAT_NULL},
    {EAT_EVENT_UART_SEND_COMPLETE,  EAT_NULL},
    {EAT_EVENT_USER_MSG,            event_threadMsg},
    {EAT_EVENT_ADC,                 EAT_NULL},
};


int event_proc(EatEvent_st* event)
{
    int i = 0;

    LOG_DEBUG("event: %s.", getEventDescription(event->event));

    for (i = 0; i < sizeof(eventProcs) / sizeof(eventProcs[0]); i++)
    {
        if (eventProcs[i].event == event->event)
        {
            EVENT_FUNC pfn = eventProcs[i].pfn;
            if (pfn)
            {
                return pfn(event);
            }
            else
            {
                LOG_ERROR("event(%s) not processed!", getEventDescription(event->event));
                return -1;
            }
        }
    }

    LOG_ERROR("event(%s) has no handler!", getEventDescription(event->event));

    return -1;
}