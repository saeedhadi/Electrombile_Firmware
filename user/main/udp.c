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

static s8 udp_socket_id = 0;

int udp_socket_connect(u8 ip_addr[4])
{
    s8 rc = SOC_SUCCESS;
    s8 val = EAT_TRUE;

    sockaddr_struct address = {SOC_SOCK_DGRAM};

    udp_socket_id = eat_soc_create(SOC_SOCK_DGRAM, NULL);
    if (udp_socket_id < 0)
    {
        LOG_ERROR("eat_soc_create return error :%d!", udp_socket_id);
        return ERR_SOCKET_CREAT_FAILED;
    }
    else
    {
        LOG_DEBUG("eat_soc_create ok, socket_id = %d.", udp_socket_id);
    }

    rc = eat_soc_setsockopt(udp_socket_id, SOC_NBIO, &val, sizeof(val));
    if (rc != SOC_SUCCESS)
    {
        LOG_ERROR("eat_soc_setsockopt set SOC_NBIO failed: %d!", rc);
        return ERR_SOCKET_OPTION_FAILED;
    }

    val = SOC_READ | SOC_WRITE | SOC_CLOSE | SOC_CONNECT;
    rc = eat_soc_setsockopt(udp_socket_id, SOC_ASYNC, &val, sizeof(val));
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
    rc = eat_soc_connect(udp_socket_id, &address);

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

static void udp_hostname_notify_cb(u32 request_id, eat_bool result, u8 ip_addr[4])
{
	if (result == EAT_TRUE)
	{
		LOG_DEBUG("hostname notify:%s -> %d.%d.%d.%d.", setting.domain, ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], setting.port);

		udp_socket_connect(ip_addr); //TODO:this should be done in function action_hostname2ip
	}
	else
	{
	    LOG_ERROR("hostname_notify_cb error: request_id = %d", request_id);//TODO: do something on this error
	}

    return;
}

int udp_socket_setup(void)
{
    s8 rc = SOC_SUCCESS;
    static int request = 0;

    if (setting.addr_type == ADDR_TYPE_IP)
    {
        return udp_socket_connect(setting.ipaddr);
    }
    else
    {
        u8 ipaddr[4] = {0};
        u8 len = 0;
        eat_soc_gethost_notify_register(udp_hostname_notify_cb);
        rc = eat_soc_gethostbyname(setting.domain, ipaddr, &len, request++);
        if (rc == SOC_WOULDBLOCK)
        {
            LOG_DEBUG("eat_soc_gethostbyname wait callback.");
            return ERR_WAITING_HOSTNAME2IP;
        }
        else if (rc == SOC_SUCCESS)
        {
            LOG_DEBUG("host:%s -> %d.%d.%d.%d:%d.", setting.domain, ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3], setting.port);
            return udp_socket_connect(ipaddr);
        }
        else
        {
            LOG_ERROR("eat_soc_gethostbyname error!");
            return ERR_GET_HOSTBYNAME_FAILED;
        }
    }
}
int udp_socket_sendData(void* data, s32 len)
{
    s32 rc;

    LOG_HEX((const char*)data, len);

    rc = eat_soc_send(udp_socket_id, data, len);
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


