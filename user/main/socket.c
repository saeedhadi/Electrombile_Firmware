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
#include "data.h"
#include "timer.h"
#include "fsm.h"
#include "error.h"

static char* getEventDescription(soc_event_enum event);
static char* getStateDescription(cbm_bearer_state_enum state);
static void hostname_notify_cb(u32 request_id, eat_bool result, u8 ip_addr[4]);


static s8 socket_id = 0;

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
#ifdef LOG_DEBUG_FLAG
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
			sprintf(soc_event, "%d", event);
			return soc_event;
		}
	}
}

/*
CBM_DEACTIVATED             = 0x01,  deactivated
CBM_ACTIVATING              = 0x02,  activating
CBM_ACTIVATED               = 0x04,  activated
CBM_DEACTIVATING            = 0x08,  deactivating
CBM_CSD_AUTO_DISC_TIMEOUT   = 0x10,  csd auto disconnection timeout
CBM_GPRS_AUTO_DISC_TIMEOUT  = 0x20,  gprs auto disconnection timeout
CBM_NWK_NEG_QOS_MODIFY      = 0x040,  negotiated network qos modify notification
CBM_WIFI_STA_INFO_MODIFY
*/
static char* getStateDescription(cbm_bearer_state_enum state)
{
	switch (state)
	{
#ifdef LOG_DEBUG_FLAG
		DESC_DEF(CBM_DEACTIVATED);
		DESC_DEF(CBM_ACTIVATING);
		DESC_DEF(CBM_ACTIVATED);
		DESC_DEF(CBM_DEACTIVATING);
		DESC_DEF(CBM_CSD_AUTO_DISC_TIMEOUT);
		DESC_DEF(CBM_GPRS_AUTO_DISC_TIMEOUT);
		DESC_DEF(CBM_NWK_NEG_QOS_MODIFY);
		DESC_DEF(CBM_WIFI_STA_INFO_MODIFY);
#endif
		default:
		{
			static char bearer_state[10] = {0};
			sprintf(bearer_state, "%d", state);
			return bearer_state;
		}
	}
}

static void hostname_notify_cb(u32 request_id, eat_bool result, u8 ip_addr[4])
{
    sockaddr_struct address={SOC_SOCK_STREAM};
    s8 rc = SOC_SUCCESS;

	if (result == EAT_TRUE)
	{
		LOG_DEBUG("hostname notify:%s -> %d.%d.%d.%d.", setting.domain, ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], setting.port);

        address.sock_type = SOC_SOCK_STREAM;
        address.addr_len = 4;
        address.port = setting.port;
        address.addr[0] = ip_addr[0];
        address.addr[1] = ip_addr[1];
        address.addr[2] = ip_addr[2];
        address.addr[3] = ip_addr[3];

        rc = eat_soc_connect(socket_id, &address);
        if(rc >= 0)
        {
        	LOG_INFO("socket id of new connection is :%d.", rc);
        }
        else if (rc == SOC_WOULDBLOCK)
        {
         	LOG_INFO("Connection is in progressing...", socket_id);
        }
        else
        {
         	LOG_ERROR("Connect return error:%d!", rc);
        }
	}

    return;
}

static void soc_notify_cb(s8 s,soc_event_enum event,eat_bool result, u16 ack_size)
{
    u8 buffer[128] = {0};
    s32 rc = 0;

    LOG_DEBUG("SOCKET notify:s(%d), socketid(%d), event(%s).", s, socket_id, getEventDescription(event));

    switch (event)
    {
        case SOC_READ:

            rc = eat_soc_recv(socket_id, buffer, 128);
            if (rc == SOC_WOULDBLOCK)
            {
                LOG_ERROR("read data from socket block!");
            }
            else if (rc > 0)
            {
                client_proc(buffer, rc);
            }
            else
            {
                LOG_ERROR("eat_soc_recv error:rc=%d!", rc);
            }

            break;

        case SOC_CONNECT:
            if(result)
            {
                LOG_INFO("SOC_CONNECT success.");
                fsm_run(EVT_SOCKET_CONNECTED);
            }
            else
            {
                LOG_ERROR("SOC_CONNECT failed:%d, maybe the server is OFF!", result);
            }

            break;

        case SOC_CLOSE:
            LOG_INFO("SOC_CLOSE.");

            eat_soc_close(socket_id);
            set_socket_state(EAT_FALSE);
            set_client_state(EAT_FALSE);

            eat_timer_start(TIMER_SOCKET, setting.socket_timer_period);
            break;

        case SOC_ACKED:
            LOG_DEBUG("acked size of send data: %d.", ack_size);
            break;

        default:
            break;
    }

}

static void bear_notify_cb(cbm_bearer_state_enum state, u8 ip_addr[4])
{
	LOG_INFO("bear_notify state: %s.", getStateDescription(state));

	switch (state)
	{
        case CBM_ACTIVATED:
		    fsm_run(EVT_BEARER_HOLD);
            break;

        case CBM_GPRS_AUTO_DISC_TIMEOUT:
            eat_reset_module();
            break;

        default:
            break;
	}
}

int socket_init(void)
{
    s8 rc = eat_gprs_bearer_open("CMNET", NULL, NULL, bear_notify_cb);
    if (rc == CBM_WOULDBLOCK)
    {
        LOG_INFO("opening bearer...");
    }
    else if (rc == CBM_OK)
    {
        LOG_INFO("open bearer success.");

        rc = eat_gprs_bearer_hold();
        if (rc == CBM_OK)
        {
            LOG_INFO("hold bearer success.");

            return socket_setup();
        }
        else
        {
            LOG_ERROR("hold bearer failed!");
            return ERR_HOLD_BEARER_FAILED;
        }
    }
    else
    {
        LOG_ERROR("open bearer failed: rc = %d", rc);

        return ERR_OPEN_BEARER_FAILED;
    }

    return SUCCESS;
}


int socket_connect()
{
    s8 rc = SOC_SUCCESS;

    sockaddr_struct address={SOC_SOCK_STREAM};

    address.sock_type = SOC_SOCK_STREAM;
    address.addr_len = 4;
    if (setting.addr_type == ADDR_TYPE_IP)
    {
        address.addr[0] = setting.ipaddr[0];
        address.addr[1] = setting.ipaddr[1];
        address.addr[2] = setting.ipaddr[2];
        address.addr[3] = setting.ipaddr[3];

        LOG_DEBUG("ip: %d.%d.%d.%d:%d.", address.addr[0], address.addr[1], address.addr[2], address.addr[3], setting.port);
    }
    else
    {
        u8 ipaddr[4] = {0};
        u8 len = 0;

        eat_soc_gethost_notify_register(hostname_notify_cb);
        rc = eat_soc_gethostbyname(setting.domain, ipaddr, &len, 1234);
        if (rc == SOC_WOULDBLOCK)
        {
            LOG_INFO("eat_soc_gethostbyname wait callback.");
            return ERR_WAITING_HOSTNAME2IP;
        }
        else if (rc == SOC_SUCCESS)
        {
            address.addr[0] = ipaddr[0];
            address.addr[1] = ipaddr[1];
            address.addr[2] = ipaddr[2];
            address.addr[3] = ipaddr[3];

            LOG_DEBUG("host:%s -> %d.%d.%d.%d:%d.", setting.domain, ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3], setting.port);
        }
        else
        {
            LOG_ERROR("eat_soc_gethostbyname error!");
            return ERR_GET_HOSTBYNAME_FAILED;
        }
    }

    address.port = setting.port;                /* TCP server port */
    rc = eat_soc_connect(socket_id, &address);
    if(rc >= 0)
    {
        LOG_INFO("socket id of new connection is :%d.", rc);
        return ERR_SOCKET_CONNECTED;
    }
    else if (rc == SOC_WOULDBLOCK)
    {
        LOG_INFO("Connection is in progressing...");
        return ERR_SOCKET_WAITING;
    }
    else
    {
        LOG_ERROR("Connect return error:%d!", rc);
        return ERR_SOCKET_FAILED;
    }
}

int socket_setup(void)
{
	s8 rc = SOC_SUCCESS;
	s8 val = EAT_TRUE;

    eat_soc_notify_register(soc_notify_cb);
    socket_id = eat_soc_create(SOC_SOCK_STREAM, 0);
    if (socket_id < 0)
    {
    	LOG_ERROR("eat_soc_create return error :%d!", socket_id);
    	return ERR_SOCKET_CREAT_FAILED;
    }
    else
    {
    	LOG_DEBUG("eat_soc_create ok, socket_id = %d.", socket_id);
    }

    rc = eat_soc_setsockopt(socket_id, SOC_NBIO, &val, sizeof(val));
    if (rc != SOC_SUCCESS)
    {
    	LOG_ERROR("eat_soc_setsockopt set SOC_NBIO failed: %d!", rc);
    	return ERR_SOCKET_OPTION_FAILED;
    }

    rc = eat_soc_setsockopt(socket_id, SOC_NODELAY, &val, sizeof(val));
    if (rc != SOC_SUCCESS)
    {
    	LOG_ERROR("eat_soc_setsockopt set SOC_NODELAY failed: %d!", rc);
    	return ERR_SOCKET_OPTION_FAILED;
    }

    val = SOC_READ | SOC_WRITE | SOC_CLOSE | SOC_CONNECT;
    rc = eat_soc_setsockopt(socket_id, SOC_ASYNC, &val, sizeof(val));
    if (rc != SOC_SUCCESS)
    {
    	LOG_ERROR("eat_soc_setsockopt set SOC_ASYNC failed: %d!", rc);
    	return ERR_SOCKET_OPTION_FAILED;
    }

    return socket_connect();
}

void socket_close(void)
{
    LOG_INFO("close the socket(%d).", socket_id);
    eat_soc_close(socket_id);

    return;
}

s32 socket_sendData(void* data, s32 len)
{
    s32 rc;

    LOG_HEX((const char*)data, len);

    rc = eat_soc_send(socket_id, data, len);
    if (rc >= 0)
    {
        LOG_DEBUG("socket send data successful.");
    }
    else
    {
        LOG_ERROR("sokcet send data failed:%d!", rc);
    }

    free_msg(data);  //TODO: is it ok to free the msg here???

    return rc;
}

