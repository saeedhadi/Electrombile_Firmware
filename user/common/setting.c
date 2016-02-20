/*
 * setting.c
 *
 *  Created on: 2015/6/24
 *      Author: jk
 */
#include <string.h>

#include <eat_fs_type.h>
#include <eat_fs.h>
#include <eat_fs_errcode.h>
#include <eat_modem.h>
#include <eat_interface.h>
#include <eat_uart.h>

#include "setting.h"
#include "debug.h"
#include "log.h"

#define SETTINGFILE_NAME  L"C:\\setting.txt"

typedef struct
{
    //Server configuration
    ADDR_TYPE addr_type;
    union
    {
        char domain[MAX_DOMAIN_NAME_LEN];
        u8 ipaddr[4];
    }addr;
    u16 port;

    //Timer configuration
    u32 gps_send_timer_period;
}STORAGE;


SETTING setting;


int cmd_deletesetting(const char* cmdString, unsigned short length)
{
    eat_fs_error_enum fs_Op_ret;

    fs_Op_ret = (eat_fs_error_enum)eat_fs_Delete(SETTINGFILE_NAME);
    if(EAT_FS_NO_ERROR!=fs_Op_ret)
    {
        LOG_ERROR("Delete settingfile Fail,and Return Error is %d",fs_Op_ret);
        return EAT_FALSE;
    }
    else
    {
        LOG_DEBUG("Delete settingfile Success");
    }
    return EAT_TRUE;
}

int cmd_catsetting(const char* cmdString, unsigned short length)
{

    FS_HANDLE fh;
    eat_bool ret = EAT_FALSE;
    int rc;

    STORAGE storage;

    LOG_INFO("cat setting...");

    fh = eat_fs_Open(SETTINGFILE_NAME, FS_READ_ONLY);
    if(EAT_FS_FILE_NOT_FOUND == fh)
    {
        LOG_INFO("setting file not exists.");
        return EAT_TRUE;
    }

    if (fh < EAT_FS_NO_ERROR)
    {
        LOG_ERROR("read setting file fail, rc: %d", fh);
        return EAT_FALSE;
    }

    rc = eat_fs_Read(fh, &storage, sizeof(STORAGE), NULL);
    if (EAT_FS_NO_ERROR == rc)
    {
        LOG_DEBUG("read setting file success.");

        if(ADDR_TYPE_DOMAIN == storage.addr_type)
        {
            LOG_DEBUG("server domain = %s:%d.", storage.addr.domain, storage.port);
        }
        else if(ADDR_TYPE_IP == storage.addr_type)
        {
            LOG_DEBUG("server ip = %d.%d.%d.%d:%d.", storage.addr.ipaddr[0], storage.addr.ipaddr[1], storage.addr.ipaddr[2], storage.addr.ipaddr[3], storage.port);
        }
        ret = EAT_TRUE;
    }
    else
    {
        LOG_ERROR("read settingfile fail, and return error: %d", fh);
    }

    eat_fs_Close(fh);

    return ret;

}


static void setting_initial(void)
{
    LOG_INFO("setting initial to default value.");

    regist_cmd("deletesetting", cmd_deletesetting);
    regist_cmd("catsetting", cmd_catsetting);

    /* Server configuration */
#if 1
    setting.addr_type = ADDR_TYPE_DOMAIN;
    strcpy(setting.domain, "www.xiaoan110.com");
#else
    setting.addr_type = ADDR_TYPE_IP;
    setting.ipaddr[0] = 121;
    setting.ipaddr[1] = 40;
    setting.ipaddr[2] = 117;
    setting.ipaddr[3] = 200;
#endif

    setting.port = 9880;

    /* Timer configuration */
    setting.main_loop_timer_period = 5000;
    setting.vibration_timer_period = 1000;
    setting.seek_timer_period = 2000;
    setting.seekautooff_timer_peroid = 30*1000;
    setting.timeupdate_timer_peroid = 24 * 60 * 60 * 1000;      //24h * 60m * 60s * 1000ms
    /* Switch configuration */
    setting.isVibrateFixed = EAT_FALSE;

    return;
}

eat_bool vibration_fixed(void)
{
    return setting.isVibrateFixed;
}

void set_vibration_state(eat_bool fixed)
{
    setting.isVibrateFixed = fixed;
}

eat_bool setting_restore(void)
{
    FS_HANDLE fh;
    eat_bool ret = EAT_FALSE;
    int rc;

    STORAGE storage;

    setting_initial();

    LOG_INFO("restore setting from file");

    /*setting reload*/
    fh = eat_fs_Open(SETTINGFILE_NAME, FS_READ_ONLY);
    if(EAT_FS_FILE_NOT_FOUND == fh)
    {
        LOG_INFO("setting file not exists.");
        return EAT_TRUE;
    }

    if (fh < EAT_FS_NO_ERROR)
    {
        LOG_ERROR("read setting file fail, rc: %d", fh);
        return EAT_FALSE;
    }

    rc = eat_fs_Read(fh, &storage, sizeof(STORAGE), NULL);
    if (EAT_FS_NO_ERROR == rc)
    {
        LOG_DEBUG("read setting file success.");

        if(storage.port != 0)
        {
            setting.port = storage.port;
        }

        if(ADDR_TYPE_DOMAIN == storage.addr_type)
        {
            setting.addr_type = ADDR_TYPE_DOMAIN;
            strncpy(setting.domain, storage.addr.domain, MAX_DOMAIN_NAME_LEN);

            LOG_DEBUG("server domain = %s:%d.", storage.addr.domain, storage.port);
        }
        else if(ADDR_TYPE_IP == storage.addr_type)
        {
            setting.addr_type = ADDR_TYPE_IP;
            setting.ipaddr[0] = storage.addr.ipaddr[0];
            setting.ipaddr[1] = storage.addr.ipaddr[1];
            setting.ipaddr[2] = storage.addr.ipaddr[2];
            setting.ipaddr[3] = storage.addr.ipaddr[3];

            LOG_DEBUG("server ip = %d.%d.%d.%d:%d.", storage.addr.ipaddr[0], storage.addr.ipaddr[1], storage.addr.ipaddr[2], storage.addr.ipaddr[3], storage.port);
        }

        if(storage.gps_send_timer_period >= 10 * 1000 && storage.gps_send_timer_period <= 6* 60 * 60 * 1000)
        {
            //FIXME: change it
//            setting.gps_send_timer_period = storage.gps_send_timer_period;
        }

        ret = EAT_TRUE;
    }
    else
    {
        LOG_ERROR("read file fail, and return error: %d", fh);
    }

    eat_fs_Close(fh);

    return ret;
}


eat_bool setting_save(void)
{
    FS_HANDLE fh, rc;
    UINT writedLen;
    eat_bool ret = EAT_FALSE;

    STORAGE storage;

    if(ADDR_TYPE_DOMAIN == setting.addr_type)
    {
        storage.addr_type = ADDR_TYPE_DOMAIN;
        strncpy(storage.addr.domain, setting.domain, MAX_DOMAIN_NAME_LEN);

        LOG_DEBUG("server domain = %s:%d.", setting.domain, setting.port);
    }
    else if(ADDR_TYPE_IP == setting.addr_type)
    {
        storage.addr_type = ADDR_TYPE_IP;
        storage.addr.ipaddr[0] = setting.ipaddr[0];
        storage.addr.ipaddr[1] = setting.ipaddr[1];
        storage.addr.ipaddr[2] = setting.ipaddr[2];
        storage.addr.ipaddr[3] = setting.ipaddr[3];

        LOG_DEBUG("server ip = %d.%d.%d.%d:%d.", setting.ipaddr[0], setting.ipaddr[1], setting.ipaddr[2], setting.ipaddr[3], setting.port);
    }
    storage.port = setting.port;
    //FIXME: change it
//    storage.gps_send_timer_period = setting.gps_send_timer_period;


    LOG_INFO("save setting...");

    fh = eat_fs_Open(SETTINGFILE_NAME, FS_READ_WRITE);
    if(EAT_FS_NO_ERROR <= fh)
    {
        LOG_INFO("open file success, fh=%d.", fh);

        rc = eat_fs_Write(fh, &storage, sizeof(STORAGE), &writedLen);
        if(EAT_FS_NO_ERROR == rc && sizeof(STORAGE) == writedLen)
        {
            LOG_DEBUG("write file success.");
        }
        else
        {
            LOG_ERROR("write file failed, and Return Error is %d, writedLen is %d.", rc, writedLen);
        }
    }
    else
    {
        LOG_ERROR("open file failed, fh=%d.", fh);
    }
    eat_fs_Close(fh);

    return ret;
}






