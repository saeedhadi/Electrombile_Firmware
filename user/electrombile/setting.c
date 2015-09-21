/*
 * setting.c
 *
 *  Created on: 2015/6/24
 *      Author: jk
 */
#include "setting.h"
#include "eat_fs_type.h"
#include "eat_fs.h"
#include "eat_fs_errcode.h"
#include "eat_modem.h"
#include "eat_interface.h"
#include "eat_uart.h"

#include "log.h"

#define SETITINGFILE_NAME  L"C:\\setting.txt"

SETTING setting =
{
    //Server configuration
	ADDR_TYPE_DOMAIN,                   //addr_type
	{
			"server.xiaoan110.com",     //domain or ipaddr
	},
	9877,                               //port

    //Timer configuration
    50000,                              //watchdog_timer_period;
    5000,                               //at_cmd_timer_period;
	30 * 1000,                          //gps_timer_period
	30 * 1000,                          //gps_send_timer_period
	1000,                               //vibration_timer_period
	30 * 1000,                          //seek_timer_period

    //Switch configuration
    EAT_FALSE,                          //isVibrateFixed
};

eat_bool vibration_fixed(void)
{
    return setting.isVibrateFixed;
}

void set_vibration_state(eat_bool fixed)
{
    setting.isVibrateFixed = fixed;
}



/*
 * read setting from flash
 */

eat_bool setting_initial(void)
{
    FS_HANDLE fh;
    FS_HANDLE seekRet;
    eat_fs_error_enum fs_Op_ret;
    UINT readLen;

    setting.addr_type = ADDR_TYPE_IP;
    setting.addr.ipaddr[0] = 120;
    setting.addr.ipaddr[1] = 25;
    setting.addr.ipaddr[2] = 157;
    setting.addr.ipaddr[3] = 233;

    fh = eat_fs_Open(SETITINGFILE_NAME, FS_READ_ONLY);
    if (fh < EAT_FS_NO_ERROR)
    {
        LOG_ERROR("Create File Fail, and Return Error is %d", fh);
        return EAT_FALSE;
    }
    else
    {
        LOG_DEBUG("eat_fs_Open():Create File Success,and FileHandle is %x", fh);

        seekRet = eat_fs_Seek(fh, 0, EAT_FS_FILE_BEGIN);
        if (0 > seekRet)
        {
            LOG_ERROR("eat_fs_Seek():Seek File Pointer Fail");
            eat_fs_Close(fh);
            return EAT_FALSE;
        }
        else
        {
            LOG_DEBUG("eat_fs_Seek():Seek File Pointer Success");
        }

        /* TODO
        fs_Op_ret = (eat_fs_error_enum) eat_fs_Read(fh, &setting, sizeof(SETTING), &readLen);
        if (EAT_FS_NO_ERROR != fs_Op_ret)
        {
            LOG_ERROR("eat_fs_Read() Fail,and Return Error: %d,Readlen is %d", fs_Op_ret, readLen);
            eat_fs_Close(fh);
            return EAT_FALSE;
        }
        else
        {
            LOG_DEBUG("eat_fs_Read():Read File Pointer Success");
            eat_fs_Close(fh);
        }
        */
    }

    return EAT_TRUE;
}

/*
 * save setting to flash
 */
eat_bool setting_save(void)
{
    FS_HANDLE fh;   //File handle
    eat_fs_error_enum fs_Op_ret;
    UINT writedLen;

    LOG_INFO("setting saving");

    fh = eat_fs_Open(SETITINGFILE_NAME, FS_CREATE_ALWAYS | FS_READ_WRITE);
    if (fh < EAT_FS_NO_ERROR)
    {
        LOG_ERROR("eat_fs_Open():Create File Fail,and Return Error is %x", fh);
        return EAT_FALSE;
    }
    else
    {
        LOG_DEBUG("eat_fs_Open():Create File Success,and FileHandle is %x", fh);

        fs_Op_ret = (eat_fs_error_enum) eat_fs_Write(fh, &setting, sizeof(SETTING), &writedLen);
        if (EAT_FS_NO_ERROR != fs_Op_ret)
        {
            LOG_ERROR("eat_fs_Write():eat_fs_Write File Fail,and Return Error is %d,Readlen is %d",fs_Op_ret, writedLen);
            eat_fs_Close(fh);
            return EAT_FALSE;
        }
        else
        {
            LOG_DEBUG("eat_fs_Write:eat_fs_Write File Success");
            eat_fs_Close(fh);
        }
    }

    return EAT_TRUE;
}
