/*
 * setting.c
 *
 *  Created on: 2015/6/24
 *      Author: jk
 */
#include <string.h>

#include "setting.h"
#include "eat_fs_type.h"
#include "eat_fs.h"
#include "eat_fs_errcode.h"
#include "eat_modem.h"
#include "eat_interface.h"
#include "eat_uart.h"
#include "log.h"



SETTING setting;
STORAGE storage;

eat_bool vibration_fixed(void)
{
    return setting.isVibrateFixed;
}

void set_vibration_state(eat_bool fixed)
{
    setting.isVibrateFixed = fixed;
}

eat_bool setting_initial(void)
{
    FS_HANDLE fh_open, fh_read;
    UINT readLen;
    eat_bool ret = EAT_FALSE;

    LOG_INFO("setting initial.");
    setting_reset();

    #if 0
    LOG_DEBUG("setting delete.");
    eat_fs_Delete(SETITINGFILE_NAME);//TODO, for debug
    #endif

    fh_open = eat_fs_Open(SETITINGFILE_NAME, FS_READ_WRITE);
    if(EAT_FS_FILE_NOT_FOUND == fh_open)
    {
        LOG_INFO("file not exists.");
        fh_open = eat_fs_Open(SETITINGFILE_NAME, FS_CREATE);
        if(EAT_FS_NO_ERROR <= fh_open)
        {
            LOG_INFO("creat file success, fh=%d.", fh_open);
            eat_fs_Close(fh_open);

            convert_setting_to_storage();
            storage_save();
            ret = EAT_TRUE;
        }
        else
        {
            LOG_ERROR("creat file failed, fh=%d.", fh_open);
        }
    }
    else if(EAT_FS_NO_ERROR <= fh_open)
    {
        LOG_INFO("open file success, fh=%d.", fh_open);

        fh_read = eat_fs_Read(fh_open, &storage, sizeof(STORAGE), &readLen);
        if (EAT_FS_NO_ERROR == fh_read)
        {
            LOG_DEBUG("read file success.");

            eat_fs_Close(fh_open);

            if(storage_check())
            {
                convert_storage_to_setting();
            }
            else
            {
                convert_setting_to_storage();
                storage_save();
            }

            ret = EAT_TRUE;
        }
        else
        {
            LOG_ERROR("read file fail, and Return Error: %d, Readlen is %d!", fh_read, readLen);
        }
    }
    else
    {
        LOG_ERROR("open file failed, fh=%d!", fh_open);
    }

    return ret;
}

void setting_reset(void)
{
    LOG_INFO("setting reset.");

    /* Server configuration */
    #if 1
    setting.addr_type = ADDR_TYPE_DOMAIN;
    strcpy(setting.addr.domain, "www.xiaoan110.com");
    #else
    setting.addr_type = ADDR_TYPE_IP;
    setting.addr.ipaddr[0] = 121;
    setting.addr.ipaddr[1] = 40;
    setting.addr.ipaddr[2] = 117;
    setting.addr.ipaddr[3] = 200;
    #endif

    setting.port = 9880;

    /* Timer configuration */
    setting.watchdog_timer_period = 50000;
    setting.at_cmd_timer_period = 5000;
    setting.gps_timer_period = 30 * 1000;
    setting.gps_send_timer_period = 30 * 1000;
    setting.vibration_timer_period = 1000;
    setting.seek_timer_period = 2000;
    setting.socket_timer_period = 60000;
    setting.heartbeat_timer_period = 3*60*1000;
    setting.seekautooff_timer_peroid = 30*1000;

    /* Switch configuration */
    setting.isVibrateFixed = EAT_FALSE;

    return;
}

eat_bool storage_check(void)
{
    if(storage.gps_send_timer_period < 10 * 1000 || storage.gps_send_timer_period > 21600 * 1000)
    {
        return EAT_FALSE;
    }

    if(storage.port == 0)
    {
        return EAT_FALSE;
    }

    return EAT_TRUE;
}

eat_bool storage_save(void)
{
    FS_HANDLE fh_open, fh_write, fh_commit;
    UINT writedLen;
    eat_bool ret = EAT_FALSE;

    LOG_INFO("storage save.");

    fh_open = eat_fs_Open(SETITINGFILE_NAME, FS_READ_WRITE);
    if(EAT_FS_NO_ERROR <= fh_open)
    {
        LOG_INFO("open file success, fh=%d.", fh_open);

        fh_write = eat_fs_Write(fh_open, &storage, sizeof(STORAGE), &writedLen);
        if(EAT_FS_NO_ERROR == fh_write && sizeof(STORAGE) == writedLen)
        {
            LOG_DEBUG("write file success.");

            fh_commit = eat_fs_Commit(fh_open);
            if(EAT_FS_NO_ERROR == fh_commit)
            {
                LOG_DEBUG("commit file success.");
                ret = EAT_TRUE;
            }
            else
            {
                LOG_ERROR("commit file failed, and Return Error is %d.", fh_commit);
            }
        }
        else
        {
            LOG_ERROR("write file failed, and Return Error is %d, writedLen is %d.", fh_write, writedLen);
        }

        eat_fs_Close(fh_open);
    }
    else
    {
        LOG_ERROR("open file failed, fh=%d.", fh_open);
    }

    return ret;
}

void convert_storage_to_setting(void)
{
    if(ADDR_TYPE_DOMAIN == storage.addr_type)
    {
        setting.addr_type = ADDR_TYPE_DOMAIN;
        strcpy(setting.addr.domain, storage.addr.domain);

        LOG_DEBUG("server domain = %s:%d.", storage.addr.domain, storage.port);
    }
    else if(ADDR_TYPE_IP == storage.addr_type)
    {
        setting.addr_type = ADDR_TYPE_IP;
        setting.addr.ipaddr[0] = storage.addr.ipaddr[0];
        setting.addr.ipaddr[1] = storage.addr.ipaddr[1];
        setting.addr.ipaddr[2] = storage.addr.ipaddr[2];
        setting.addr.ipaddr[3] = storage.addr.ipaddr[3];

        LOG_DEBUG("server ip = %d.%d.%d.%d:%d.", storage.addr.ipaddr[0], storage.addr.ipaddr[1], storage.addr.ipaddr[2], storage.addr.ipaddr[3], storage.port);
    }
    setting.port = storage.port;
    setting.gps_send_timer_period = storage.gps_send_timer_period;

    LOG_DEBUG("gps_send_timer_period = %d.", storage.gps_send_timer_period);

    return;
}

void convert_setting_to_storage(void)
{
    if(ADDR_TYPE_DOMAIN == setting.addr_type)
    {
        storage.addr_type = ADDR_TYPE_DOMAIN;
        strcpy(storage.addr.domain, setting.addr.domain);

        LOG_DEBUG("server domain = %s:%d.", setting.addr.domain, setting.port);
    }
    else if(ADDR_TYPE_IP == setting.addr_type)
    {
        storage.addr_type = ADDR_TYPE_IP;
        storage.addr.ipaddr[0] = setting.addr.ipaddr[0];
        storage.addr.ipaddr[1] = setting.addr.ipaddr[1];
        storage.addr.ipaddr[2] = setting.addr.ipaddr[2];
        storage.addr.ipaddr[3] = setting.addr.ipaddr[3];

        LOG_DEBUG("server ip = %d.%d.%d.%d:%d.", setting.addr.ipaddr[0], setting.addr.ipaddr[1], setting.addr.ipaddr[2], setting.addr.ipaddr[3], setting.port);
    }
    storage.port = setting.port;
    storage.gps_send_timer_period = setting.gps_send_timer_period;

    LOG_DEBUG("gps_send_timer_period = %d.", storage.gps_send_timer_period);

    return;
}


