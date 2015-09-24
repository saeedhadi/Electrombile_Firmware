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

typedef int (*EVENT_FUNC)(const EatEvent_st* event);
typedef struct
{
	EatEvent_enum event;
	EVENT_FUNC pfn;
}EVENT_PROC;


/*
 * local event function definition
 */
int event_timer(const EatEvent_st* event);
int event_threadMsg(const EatEvent_st* event);
int event_mod_ready_rd(const EatEvent_st* event);

static EVENT_PROC eventProcs[] =
{
    {EAT_EVENT_TIMER,				event_timer},
    {EAT_EVENT_MDM_READY_RD,        event_mod_ready_rd},
    {EAT_EVENT_MDM_READY_WR,        EAT_NULL},
    {EAT_EVENT_UART_READY_RD,       event_uart_ready_rd},
    {EAT_EVENT_UART_READY_WR,		EAT_NULL},
    {EAT_EVENT_UART_SEND_COMPLETE,	EAT_NULL},
    {EAT_EVENT_USER_MSG,            event_threadMsg},
};


#define DESC_DEF(x) case x:\
                            return #x

static char* getEventDescription(EatEvent_enum event)
{
    switch (event)
    {
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
        default:
        {
            static char soc_event[10] = {0};
            sprintf(soc_event, "%d", event);
            return soc_event;
        }
    }
}

int event_mod_ready_rd(const EatEvent_st* event)
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


int event_proc(EatEvent_st* event)
{
	int i = 0;

    LOG_DEBUG("event: %s", getEventDescription(event->event));

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
		        LOG_ERROR("event(%d) not processed!", event->event);
			}
		}
	}

	return -1;
}

int event_timer(const EatEvent_st* event)
{
    switch (event->data.timer.timer_id)
    {
        case TIMER_WATCHDOG:
            LOG_INFO("TIMER_WATCHDOG expire!");
            feedWatchdog();
            eat_timer_start(event->data.timer.timer_id, setting.watchdog_timer_period);
            break;

        case TIMER_AT_CMD:
            LOG_INFO("TIMER_AT_CMD expire!");
            eat_modem_write("AT+CGATT?\n", 10);
            eat_timer_start(event->data.timer.timer_id, setting.at_cmd_timer_period);
            break;

        case TIMER_GPS_SEND:
            LOG_INFO("TIMER_GPS_SEND expire!");
            eat_timer_start(event->data.timer.timer_id, setting.gps_send_timer_period);
            client_loop();
            break;

        default:
            LOG_ERROR ("timer(%d) not processed", event->data.timer.timer_id);
            break;
    }
    return 0;
}

int event_threadMsg(const EatEvent_st* event)
{
    MSG_THREAD* msg = (MSG_THREAD*) event->data.user_msg.data_p;
    u8 msgLen = event->data.user_msg.len;

    switch (msg->cmd)
    {
        case CMD_THREAD_GPS:
        {
            LOCAL_GPS* gps = (LOCAL_GPS*) msg->data;

            if (msgLen < sizeof(LOCAL_GPS)  || !gps)
            {
                LOG_ERROR("msg from THREAD_GPS error!");
                break;
            }

            data.isGpsFixed = gps->isGpsFixed;

            if (gps->isGpsFixed)    //update the local GPS data
            {
                data.gps.latitude = gps->gps.latitude;
                data.gps.longitude = gps->gps.longitude;
                LOG_DEBUG("receive thread command CMD_GPS_UPDATE: lat(%f), lng(%f).", gps->gps.latitude, gps->gps.longitude);
            }
            else    //update local cell info
            {
                data.cgi.mcc = gps->cellInfo.mcc;
                data.cgi.mnc = gps->cellInfo.mnc;
                data.cgi.cellNo = gps->cellInfo.cellNo;
                memcpy(data.cells, gps->cellInfo.cell, sizeof(CELL) * gps->cellInfo.cellNo);
                LOG_DEBUG("receive thread command CMD_GPS_UPDATE: cellid(%x), lac(%d).", data.cells[0].cellid, data.cells[0].lac);
            }
            break;
        }

        case CMD_THREAD_SMS:
            LOG_DEBUG("receive thread command CMD_SMS.");
            break;

        case CMD_THREAD_VIBRATE:
        {
            unsigned char* alarm_type = (unsigned char*)msg->data;
            MSG_ALARM_REQ* socket_msg;

            if (msgLen != 1)
            {
                LOG_ERROR("msg length error: msgLen(%d)!", msgLen);
                break;
            }
            LOG_DEBUG("receive thread command CMD_VIBRATE: alarmType(%d).", *alarm_type);

            socket_msg = alloc_msg(CMD_ALARM, sizeof(MSG_ALARM_REQ));
            if (!socket_msg)
            {
                LOG_ERROR("alloc message failed!");
                break;
            }

            LOG_DEBUG("send alarm vibrate message.");
            socket_msg->alarmType = *alarm_type;
            socket_sendData(socket_msg, sizeof(MSG_ALARM_REQ));
            break;
        }

        case CMD_THREAD_SEEK:
        {
            SEEK_INFO* seek = (SEEK_INFO*)msg->data;
            MSG_433* seek_msg;

            if (msgLen < sizeof(SEEK_INFO)  || !seek)
            {
                LOG_ERROR("msg from THREAD_SEEK error!");
                break;
            }

            LOG_DEBUG("receive thread command CMD_SEEK: value(%f).", seek->intensity);

            seek_msg = alloc_msg(CMD_433, sizeof(MSG_433));
            if (!seek_msg)
            {
                LOG_ERROR("alloc message failed!");
                break;
            }

            LOG_DEBUG("send seek value message.");
            seek_msg->intensity = htons((int)seek->intensity);
            socket_sendData(seek_msg, sizeof(MSG_433));
            break;
        }

        default:
            LOG_ERROR("receive unknown thread command:%d!", msg->cmd);
            break;
    }

    freeMsg(msg);

    return 0;
}

