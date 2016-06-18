/*
 * socket.c
 *
 *  Created on: 2015/7/8/
 *      Author: jk
 */
#include <stdio.h>
#include <eat_interface.h>
#include <eat_socket.h>

#include "socket.h"
#include "setting.h"
#include "log.h"
#include "client.h"
#include "msg.h"
#include "timer.h"
#include "fsm.h"
#include "error.h"
#include "msg_queue.h"


static s8 socket_id_udp = 0;

static u32 request_id = 0;

#define DESC_DEF(x)	case x:\
                        return #x

/*
SOC_READ    = 0x01,   Notify for read
SOC_WRITE   = 0x02,   Notify for write
SOC_ACCEPT  = 0x04,   Notify for accept
SOC_CONNECT = 0x08,   Notify for connect
SOC_CLOSE   = 0x10,   Notify for close
SOC_ACKED   = 0x20,   Notify for acked
*/
static char* getEventDescription(soc_event_enum event)
{
	switch (event)
	{
#ifdef APP_DEBUG
		DESC_DEF(SOC_READ);
		DESC_DEF(SOC_WRITE);
		DESC_DEF(SOC_ACCEPT);
		DESC_DEF(SOC_CONNECT);
		DESC_DEF(SOC_CLOSE);
		DESC_DEF(SOC_ACKED);
#endif
		default:
		{
			static char soc_event[10] = {0};
			snprintf(soc_event, 10, "%d", event);
			return soc_event;
		}
	}
}

static void soc_notify_cb_udp(s8 s,soc_event_enum event,eat_bool result, u16 ack_size)
{
    u8 buffer[1152] = {0};//1K + 128 for upgrade module
    s32 rc = 0;

    LOG_DEBUG("SOCKET notify:socketid(%d), event(%s).", s, getEventDescription(event));

    switch (event)
    {
        case SOC_READ:

            rc = eat_soc_recv(socket_id_udp, buffer, 1152);//1K + 128 for upgrade module
            if (rc > 0)
            {
                client_proc(buffer, rc);
            }
            else
            {
                LOG_ERROR("eat_soc_recv error:rc=%d!", rc);
            }

            break;

        case SOC_CONNECT:
                LOG_DEBUG("SOC_CONNECT success.");
            break;

        case SOC_CLOSE:
            LOG_INFO("SOC_CLOSE:socketid = %d", s);
            eat_soc_close(s);
            break;

        case SOC_ACKED:
            LOG_DEBUG("acked size of send data: %d.", ack_size);
            break;

        default:
            LOG_INFO("SOC_NOTIFY %d not handled", event);
            break;
    }

}


int socket_connect_udp(void)
{
    s8 rc = SOC_SUCCESS;
    s8 val = EAT_TRUE;

    sockaddr_struct address = {SOC_SOCK_DGRAM};

    socket_id_udp = eat_soc_create(SOC_SOCK_DGRAM, NULL);
    if (socket_id_udp < 0)
    {
        LOG_ERROR("eat_soc_create return error :%d!", socket_id_udp);
        return ERR_SOCKET_CREAT_FAILED;
    }
    else
    {
        LOG_DEBUG("eat_soc_create ok, socket_id = %d.", socket_id_udp);
    }

    rc = eat_soc_setsockopt(socket_id_udp, SOC_NBIO, &val, sizeof(val));
    if (rc != SOC_SUCCESS)
    {
        LOG_ERROR("eat_soc_setsockopt set SOC_NBIO failed: %d!", rc);
        return ERR_SOCKET_OPTION_FAILED;
    }

    val = SOC_READ | SOC_WRITE | SOC_CLOSE | SOC_CONNECT;
    rc = eat_soc_setsockopt(socket_id_udp, SOC_ASYNC, &val, sizeof(val));
    if (rc != SOC_SUCCESS)
    {
        LOG_ERROR("eat_soc_setsockopt set SOC_ASYNC failed: %d!", rc);
        return ERR_SOCKET_OPTION_FAILED;
    }



    address.sock_type = SOC_SOCK_DGRAM;
    address.addr_len = 4;

    address.addr[0] = 121;//ip_addr[0];
    address.addr[1] = 42;//ip_addr[1];
    address.addr[2] = 38;//ip_addr[2];
    address.addr[3] = 93;//ip_addr[3];

    LOG_DEBUG("ip: %d.%d.%d.%d:%d.", address.addr[0], address.addr[1], address.addr[2], address.addr[3], setting.port_udp);


    address.port = setting.port_udp;                /* UDP server port */
    rc = eat_soc_connect(socket_id_udp, &address);

    if(rc >= 0)
    {
        LOG_DEBUG("socket id of new connection is :%d.", rc);
        return ERR_SOCKET_CONNECTED;
    }
    else if (rc == SOC_WOULDBLOCK)
    {
        LOG_DEBUG("Connection is in progressing...");
        return ERR_SOCKET_WAITING;
    }
    else
    {
        LOG_ERROR("Connect return error:%d!", rc);
        return ERR_SOCKET_FAILED;
    }
}

int socket_sendData_udp(void* data, s32 len)
{
    s32 rc;

    LOG_HEX((const char*)data, len);

    rc = eat_soc_send(socket_id_udp, data, len);
    if (rc >= 0)
    {
        LOG_DEBUG("socket send data successful.");
    }
    else
    {
        LOG_ERROR("sokcet send data failed:%d!", rc);
    }

    free_msg(data);

    return rc;
}


