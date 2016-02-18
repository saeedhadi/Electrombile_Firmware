/*
 * response.c
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */
#include <string.h>

#include <eat_interface.h>

#include "response.h"
#include "msg.h"
#include "log.h"
#include "thread_msg.h"
#include "thread.h"
#include "socket.h"
#include "fsm.h"
#include "version.h"

//TODO: the following header file should be removed
#include "timer.h"
#include "setting.h"
#include "data.h"
#include "mileage.h"


int cmd_Login_rsp(const void* msg)
{
    LOG_DEBUG("get login respond.");

    fsm_run(EVT_LOGINED);

    return 0;
}

int cmd_Ping_rsp(const void* msg)
{
    LOG_DEBUG("get ping respond.");

    return 0;
}

int cmd_Alarm_rsp(const void* msg)
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

int cmd_Sms_rsp(const void* msg)
{
    return 0;
}

int cmd_Seek_rsp(const void* msg)
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

int cmd_Location_rsp(const void* msgLocation)
{
    u8 msgLen = sizeof(MSG_THREAD);
    MSG_THREAD* msg = allocMsg(msgLen);

    msg->cmd = CMD_THREAD_LOCATION;
    msg->length = 0;

    LOG_DEBUG("send CMD_THREAD_LOCATION to THREAD_GPS.");
    sendMsg(THREAD_MAIN, THREAD_GPS, msg, msgLen);

    return 0;
}

int cmd_AutodefendSwitchSet_rsp(const void* msg)
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

int cmd_AutodefendSwitchGet_rsp(const void* msg)
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

int cmd_AutodefendPeriodSet_rsp(const void* msg)
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

int cmd_AutodefendPeriodGet_rsp(const void* msg)
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

int cmd_Battery_rsp(const void* msg)
{
    MSG_BATTERY_RSP* req = (MSG_BATTERY_RSP*)msg;
    MSG_BATTERY_RSP* rsp = NULL;

    rsp = alloc_rspMsg(&req->header);
    if (!rsp)
    {
        LOG_ERROR("alloc baterry rsp message failed!");
        return -1;
    }
    rsp->miles = get_mileage();
    rsp->percent = get_battery();
    socket_sendData(rsp, sizeof(MSG_BATTERY_RSP));

    return 0;

}


int cmd_Defend_rsp(const void* msg)
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

int cmd_Timer_rsp(const void* msg)
{
    MSG_GPSTIMER_REQ* req = (MSG_GPSTIMER_REQ*)msg;
    MSG_GPSTIMER_RSP* rsp = NULL;
    if(0 >= req->timer)
    {
        ;//rsp at the end
    }
    else if(10 >= req->timer)
    {
        setting_save();

    }
    else if(21600 <= req->timer)
    {
        setting_save();

    }
    else if((10 < req->timer)&&(21600 > req->timer))
    {

        setting_save();

    }
    rsp = alloc_rspMsg(&req->header);

    //TODO: fix the gps upload time
    rsp->result = 30;
    socket_sendData(rsp,sizeof(MSG_GPSTIMER_RSP));

    return 0;
}
int cmd_Server_rsp(const void* msg)
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

        setting_save();
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

            setting_save();
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

int cmd_UpgradeStart_rsp(const void* msg)
{
    MSG_UPGRADE_START* req = (MSG_UPGRADE_START*)msg;
    MSG_UPGRADE_START_RSP* rsp = NULL;

    if (req->version <= VERSION)    //No need to upgrade, normally not happened
    {
        rsp = alloc_rspMsg(msg);
        if (!rsp)
        {
            LOG_ERROR("alloc rsp msg failed:cmd=%d", req->header.cmd);
            return -1;
        }
        rsp->code = -1;
        socket_sendData(rsp,sizeof(MSG_UPGRADE_START_RSP));
        return -1;
    }

    //TODO: 调用eat_fs_GetFolderSize获取磁盘剩余空间大小，是否足以容纳 升级包大小(req->size)
    if (0)
    {
        //返回错误
        return -1;
    }

    //创建升级包文件，回应可以升级

    return 0;
}

int cmd_UpgradeData_rsp(const void* msg)
{
    MSG_UPGRADE_DATA* req = (MSG_UPGRADE_DATA*)msg;
    MSG_UPGRADE_DATA_RSP* rsp = NULL;

    //TODO: complete the following procedure
    //打开升级包文件
    //fseek to req->offset
    //fwrite req->data
    //response req->offset + length of req->data

    return 0;
}

int cmd_UpgradeEnd_rsp(const void* msg)
{
    MSG_UPGRADE_END* req = (MSG_UPGRADE_END*)msg;
    MSG_UPGRADE_END_RSP* rsp = NULL;

    //TODO: complete the following procedure
    //校验升级包的大小是否和req->size一致
    //校验升级包的校验和是否和req->checksum一致
    //响应消息

    //启动升级

    return 0;
}
