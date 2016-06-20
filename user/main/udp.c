/*
 * udp.c
 *
 *  Created on: 2016/6/18/
 *      Author: lc
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

int socket_connect_udp(u8 ip_addr[4])
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

    address.addr[0] = ip_addr[0];
    address.addr[1] = ip_addr[1];
    address.addr[2] = ip_addr[2];
    address.addr[3] = ip_addr[3];

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

int socket_sendData_UDP(void* data, s32 len)
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


