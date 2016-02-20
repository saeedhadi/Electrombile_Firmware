/*
 * upgrade.c
 *
 *  Created on: 2016年2月18日
 *      Author: jk
 */

#include <eat_fs.h>

#include "upgrade.h"
#include "log.h"

#define UPGRADE_FILE_NAME  L"C:\\app.bin"


u8 * get_AppFile(int *filesize)
{
    FS_HANDLE FileHandle;
    int rc;
    u8* app_buf = NULL;

    FileHandle  = eat_fs_Open(UPGRADE_FILE_NAME, FS_READ_ONLY);
    if(EAT_FS_NO_ERROR <= FileHandle)
    {
        LOG_DEBUG("open file success, fh=%d.", FileHandle);
    }
    else
    {
        LOG_ERROR("open file failed, fh=%d.", FileHandle);
        return 0;
    }

    rc = eat_fs_GetFileSize(FileHandle,filesize);
    if(EAT_FS_NO_ERROR != rc)
    {
        LOG_ERROR("get file size error , and return error is :%d",rc);
        eat_fs_Close(FileHandle);
        return 0;
    }
    else
    {
        LOG_DEBUG("get file size success , file size is:%d",*filesize);
    }

    app_buf = eat_mem_alloc(*filesize);
    if (!app_buf)
    {
        LOG_ERROR("alloc app_buf error!");
        return 0;
    }

    rc = eat_fs_Read(FileHandle,app_buf,*filesize, NULL);
    if (EAT_FS_NO_ERROR == rc)
    {
        LOG_DEBUG("read app file success.");
    }
    else
    {
        LOG_ERROR("read file fail, and return error: %d", rc);
        return 0;
    }

    eat_fs_Close(FileHandle);
    return app_buf;
}


/*
*fun:create the upgrade file
*return:0 express success ; -1 express fail
*/
int upgrade_createFile(void)
{
    FS_HANDLE fh;

    LOG_INFO("create upgrade file...");

    fh = eat_fs_Open(UPGRADE_FILE_NAME,FS_CREATE);
    if(EAT_FS_NO_ERROR <= fh)
    {
        LOG_DEBUG("create file success, fh=%d.", fh);
    }
    else
    {
        LOG_ERROR("create file failed, fh=%d.", fh);
        return -1;
    }

    eat_fs_Close(fh);

    return 0;
}
/*
*fun:receive the upgrade data and write it in file
*note:offset is not in use now , just seek to endfile
*/
int upgrade_appendFile(int offset, char* data,  unsigned int length)
{
    FS_HANDLE fh_open, fh_write, fh_commit,seekRet;
    UINT writedLen;
    int rc = 0;

    fh_open = eat_fs_Open(UPGRADE_FILE_NAME, FS_READ_WRITE);

    if(EAT_FS_NO_ERROR <= fh_open)
    {
        LOG_INFO("create or open log_file success, fh=%d.", fh_open);

        seekRet = eat_fs_Seek(fh_open,0,EAT_FS_FILE_END);

        if(seekRet < 0)
        {
            LOG_ERROR("Seek File Pointer Fail");
            rc = -1;
        }
        else
        {
            LOG_INFO("Seek File Pointer Success");

            fh_write = eat_fs_Write(fh_open, data, length, &writedLen);
            if((EAT_FS_NO_ERROR == fh_write) && length == writedLen)
            {
                fh_commit = eat_fs_Commit(fh_open);//don't know how soon the next block will come , commit it
                if(EAT_FS_NO_ERROR == fh_commit)
                {
                    LOG_INFO("commit file success.");
                }
                else
                {
                    LOG_ERROR("commit file failed, and Return Error is %d.", fh_commit);
                    rc = -1;
                }
            }
            else
            {
                LOG_ERROR("write file failed,Error is%d",fh_write);
                rc =-1;
            }
        }
    }
    else
    {
        LOG_ERROR("open file failed, fh=%d!", fh_open);
        rc = -1;
    }

    eat_fs_Close(fh_open);
    return rc;

}

int upgrade_do(char* app_data)
{
    u32 APP_DATA_RUN_BASE;  //app run addr
    u32 APP_DATA_STORAGE_BASE;  //app data storage addr
    u32 app_space_value;
    unsigned char *addr;
    int app_dataLen;
    int rc;

    app_dataLen = strlen(app_data);

    APP_DATA_RUN_BASE = eat_get_app_base_addr(); //get app addr
    LOG_INFO("APP_DATA_RUN_BASE : %ld",APP_DATA_RUN_BASE);

    app_space_value = eat_get_app_space();  //get app space size
    LOG_INFO("app_space_value : %ld",app_space_value);

    APP_DATA_STORAGE_BASE = APP_DATA_RUN_BASE + (app_space_value>>1);//second half is space use to storage app_upgrade_data

    addr = (unsigned char *)(APP_DATA_STORAGE_BASE);

    rc = eat_flash_erase(addr , app_dataLen);//erase the flash to write new app_data_storage
    if(EAT_FALSE == rc)
    {
        LOG_ERROR("Erase flash failed [0x%08x, %dKByte]", APP_DATA_STORAGE_BASE,  app_dataLen/1024);
        return -1;
    }

    rc = eat_flash_write(addr , app_data , app_dataLen);//write the new app_data_storage
    if(EAT_FALSE == rc)
    {
        LOG_ERROR("Write Flash Failed.");
        return -1;
    }

    //upgrade app
    eat_update_app((void*)(APP_DATA_RUN_BASE),(void*)(APP_DATA_STORAGE_BASE), app_dataLen, EAT_PIN_NUM, EAT_PIN_NUM,EAT_FALSE);


    LOG_DEBUG("Upgrade App Over!");

    return 0;
}


