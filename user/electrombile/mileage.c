
#include <string.h>

#include <eat_fs_type.h>
#include <eat_fs.h>
#include <eat_fs_errcode.h>
#include <eat_modem.h>
#include <eat_interface.h>
#include <eat_uart.h>

#include "setting.h"
#include "log.h"
#include "mileage.h"


DumpVoltage mileage_storage = {0};

eat_bool mileage_restore(void)
{
    FS_HANDLE fh;
    eat_bool ret = EAT_FALSE;
    int rc , i;

    DumpVoltage storage;

    mileage_initial();

    LOG_INFO("restore mileage from file");

    /*mileage reload*/
    fh = eat_fs_Open(MILEAGEFILE_NAME, FS_READ_ONLY);
    if(EAT_FS_FILE_NOT_FOUND == fh)
    {
        LOG_INFO("mileage file not exists.");
        return EAT_TRUE;
    }

    if (fh < EAT_FS_NO_ERROR)
    {
        LOG_ERROR("read mileage file fail, rc: %d", fh);
        return EAT_FALSE;
    }

    rc = eat_fs_Read(fh, &storage, sizeof(DumpVoltage), NULL);
    if (EAT_FS_NO_ERROR == rc)
    {
        LOG_DEBUG("read mileage file success.");
        for(i = 0;i < MAX_MILEAGE_LARGE;i++)
        {
            mileage_storage.dump_mileage[i] = storage.dump_mileage[i];
            mileage_storage.voltage[i] = storage.dump_mileage[i];
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


void mileage_initial(void)
{
    int i;
    LOG_INFO("setting initial to default value.");

    for(i = 0;i <MAX_MILEAGE_LARGE;i++)
    {
        mileage_storage.dump_mileage[i] = 0;
        mileage_storage.voltage[i] = (48-12*i/MAX_MILEAGE_LARGE)*3/103;
    }
}

eat_bool mileage_save(void)
{
    FS_HANDLE fh;
    int rc,i;
    UINT writedLen;
    eat_bool ret = EAT_FALSE;

    DumpVoltage storage;

    for(i = 0;i < MAX_MILEAGE_LARGE; i++)
    {
        storage.dump_mileage[i] = mileage_storage.dump_mileage[i];
        storage.voltage[i] = mileage_storage.voltage[i];
    }


    LOG_INFO("save mileage...");

    fh = eat_fs_Open(MILEAGEFILE_NAME, FS_READ_WRITE);
    if(EAT_FS_NO_ERROR <= fh)
    {
        LOG_DEBUG("open file success, fh=%d.", fh);

        rc = eat_fs_Write(fh, &storage, sizeof(DumpVoltage), &writedLen);
        if(EAT_FS_NO_ERROR == rc && sizeof(DumpVoltage) == writedLen)
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




















