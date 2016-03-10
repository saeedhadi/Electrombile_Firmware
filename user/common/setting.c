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
#include "version.h"
#include "log.h"
#include "fs.h"
#include "cJSON.h"
#include "mem.h"

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

#define TAG_SERVER  "SERVER"
#define TAG_ADDR_TYPE   "ADDR_TYPE"
#define TAG_ADDR   "ADDR"
#define TAG_PORT    "PORT"


static void setting_initial(void)
{
    cJSON_Hooks mem_hooks;

    mem_hooks.malloc_fn = malloc;
    mem_hooks.free_fn = free;

    LOG_DEBUG("setting initial to default value.");

    //initial the cJSON memory hook
    cJSON_InitHooks(&mem_hooks);

    /* Server configuration */
#if 1
    setting.addr_type = ADDR_TYPE_DOMAIN;
    strcpy(setting.domain, "www.xiaoan110.com");
#else
    setting.addr_type = ADDR_TYPE_IP;
    setting.ipaddr[0] = 121;
    setting.ipaddr[1] = 42;
    setting.ipaddr[2] = 38;
    setting.ipaddr[3] = 93;
#endif

    setting.port = 9880;

    /* Timer configuration */
    setting.main_loop_timer_period = 5000;
    setting.vibration_timer_period = 1000;
    setting.seek_timer_period = 2000;
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
    UINT filesize = 0;
    char *buf = 0;
    cJSON *conf = 0;
    cJSON *addr = 0;

    setting_initial();

    LOG_DEBUG("restore setting from file");

    //open the file
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

    //get the file length
    rc = eat_fs_GetFileSize(fh, &filesize);
    if(EAT_FS_NO_ERROR != rc)
    {
        LOG_ERROR("get file size error, return %d",rc);
        eat_fs_Close(fh);
        return EAT_FALSE;
    }
    else
    {
        LOG_DEBUG("get file size success , file size %d",filesize);
    }

    //malloc memory to read the file
    buf = malloc(filesize);
    if (!buf)
    {
        LOG_ERROR("malloc file content buffer failed");
        eat_fs_Close(fh);
        return EAT_FALSE;
    }


    //read the file
    rc = eat_fs_Read(fh, buf, filesize, NULL);
    if (rc != EAT_FS_NO_ERROR)
    {
        LOG_ERROR("read file fail, and return error: %d", fh);
        eat_fs_Close(fh);
        return EAT_FALSE;
    }

    //parse the JSON data
    conf = cJSON_Parse(buf);
    if (!conf)
    {
        LOG_ERROR("setting config file format error!");
        eat_fs_Close(fh);
        return EAT_FALSE;
    }

    addr = cJSON_GetObjectItem(conf, TAG_SERVER);
    setting.addr_type = cJSON_GetObjectItem(addr, TAG_ADDR_TYPE)->valueint;
    if (setting.addr_type == ADDR_TYPE_DOMAIN)
    {
        char *domain = cJSON_GetObjectItem(addr, TAG_ADDR)->valuestring;
        strncpy(setting.domain, domain, MAX_DOMAIN_NAME_LEN);
    }
    else
    {
        char *ip = cJSON_GetObjectItem(addr, TAG_ADDR)->valuestring;
        int count = sscanf(ip,"%u.%u.%u.%u", setting.ipaddr, setting.addr_type + 1, setting.addr_type +2, setting.ipaddr + 3);
        if (count != 4) //4 means got four number of ip
        {
            LOG_ERROR("restore ip address failed");
            return EAT_FALSE;
        }
    }

    setting.port = cJSON_GetObjectItem(addr, TAG_PORT)->valueint;

    ret = EAT_TRUE;


    eat_fs_Close(fh);

    return ret;
}


eat_bool setting_save(void)
{
    FS_HANDLE fh, rc;
    eat_bool ret = EAT_FALSE;

    cJSON *root = cJSON_CreateObject();
    cJSON *address = cJSON_CreateObject();
    cJSON *autolock = cJSON_CreateObject();

    char *content = 0;


    cJSON_AddNumberToObject(address, TAG_ADDR_TYPE, setting.addr_type);
    if (setting.addr_type == ADDR_TYPE_DOMAIN)
    {
        cJSON_AddStringToObject(address, "ADDR", setting.domain);
    }
    else
    {
        char server[MAX_DOMAIN_NAME_LEN] = 0;
        snprintf(server, MAX_DOMAIN_NAME_LEN, "%d.%d.%d.%d", setting.ipaddr[0], setting.ipaddr[1], setting.ipaddr[2], setting.ipaddr[3]);
        cJSON_AddStringToObject(address, TAG_ADDR, server);
    }
    cJSON_AddNumberToObject(address, TAG_PORT, setting.port);

    cJSON_AddItemToObject(root, TAG_SERVER, address);

    //TODO: the lock switch and the autolock switch plus the auto lock period

    content = cJSON_Print(root);
    LOG_DEBUG("save setting...");

    fh = eat_fs_Open(SETTINGFILE_NAME, FS_READ_WRITE|FS_CREATE);
    if(EAT_FS_NO_ERROR <= fh)
    {
        LOG_DEBUG("open file success, fh=%d.", fh);

        rc = eat_fs_Write(fh, content, strlen(content), 0);
        if(EAT_FS_NO_ERROR == rc)
        {
            LOG_DEBUG("write file success.");
        }
        else
        {
            LOG_ERROR("write file failed, and Return Error is %d", rc);
        }
    }
    else
    {
        LOG_ERROR("open file failed, fh=%d.", fh);
    }

    free(content);
    cJSON_Delete(root);
    eat_fs_Close(fh);

    return ret;
}






