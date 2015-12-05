/*
 * log.c
 *
 *  Created on: 2015/10/16
 *      Author: jk
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "uart.h"
#include "data.h"
#include "log.h"
#include "client.h"





void log_hex(const char* data, int length)
{
    int i = 0, j = 0;

    print("    ");
    for (i  = 0; i < 16; i++)
    {
        print("%X  ", i);
    }
    print("    ");
    for (i = 0; i < 16; i++)
    {
        print("%X", i);
    }

    print("\r\n");

    for (i = 0; i < length; i += 16)
    {
        print("%02d  ", i / 16 + 1);
        for (j = i; j < i + 16 && j < length; j++)
        {
            print("%02x ", data[j] & 0xff);
        }
        if (j == length && length % 16)
        {
            for (j = 0; j < (16 - length % 16); j++)
            {
                print("   ");
            }
        }
        print("    ");
        for (j = i; j < i + 16 && j < length; j++)
        {
            if (data[j] < 32)
            {
                print(".");
            }
            else
            {
                print("%c", data[j] & 0xff);
            }
        }

        print("\r\n");
    }
}

void log_remote(const char* fmt, ...) //when socket connected,LOG_remote;else LOG_location
{
    char buf[1024] = {0};
    int length = 0;

    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, 1024, fmt, arg);
    va_end(arg);

    length = strlen(buf);

    if (socket_conneted())
    {
        msg_wild(buf, length);
    }
    else
    {
        eat_trace("%s", buf);
    }
}

void log_file(const char* fmt, ...)
{
    //eat_bool ret = EAT_FALSE;
    char buf[1024] = "\0";
    FS_HANDLE fh_open, fh_write, fh_commit,seekRet;
    //int seekRet = 0;
    UINT writedLen;

    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, 1024, fmt, arg);
    va_end(arg);


    strcpy(buf+strlen(buf),"\n\0");

    fh_open = eat_fs_Open(LOGFILE_NAME, FS_READ_WRITE|FS_CREATE);

    if(EAT_FS_NO_ERROR <= fh_open)
    {
        LOG_INFO("create or open log_file success, fh=%d.", fh_open);

        seekRet = eat_fs_Seek(fh_open,0,EAT_FS_FILE_END);

        if(seekRet < 0)
        {
            LOG_ERROR("Seek File Pointer Fail");

            eat_fs_Close(fh_open);

            return;
        }
        else
        {
            LOG_INFO("Seek File Pointer Success");

            fh_write = eat_fs_Write(fh_open, buf, strlen(buf), &writedLen);

            if((EAT_FS_NO_ERROR == fh_write) && (strlen(buf) == writedLen))
            {

                fh_commit = eat_fs_Commit(fh_open);
                if(EAT_FS_NO_ERROR == fh_commit)
                {
                    LOG_DEBUG("commit file success.");
                }
                else
                {
                    LOG_ERROR("commit file failed, and Return Error is %d.", fh_commit);
                }
            }
            else
            {
                LOG_ERROR("write file failed,Error is%d, writedLen is%d,strlen(buf) is%d",fh_write,writedLen,strlen(buf));
            }
            eat_fs_Close(fh_open);
            return;
        }

    }
    else
    {
        LOG_ERROR("open file failed, fh=%d!", fh_open);
        eat_fs_Close(fh_open);
        return;
    }

}


void read_file(unsigned short *filename)
{
    FS_HANDLE fh_open,fh_read;
    char buf[1024] = "\0";
    int seekRet = NULL;
    UINT count_line = 0,readLen = 0;

    fh_open = eat_fs_Open(LOGFILE_NAME, FS_READ_ONLY);

    if(EAT_FS_FILE_NOT_FOUND == fh_open)
    {
        LOG_ERROR("log_file not exists.");
        return;
    }
    else if(EAT_FS_NO_ERROR <= fh_open)
    {
        LOG_INFO("open log_file success, fh=%d.", fh_open);

        seekRet = eat_fs_Seek(fh_open,0,EAT_FS_FILE_BEGIN);

        if(0 <= seekRet)
        {
            LOG_INFO("Seek File Pointer Success");

            fh_read = eat_fs_Read(fh_open,&buf,1024, &readLen);

            if(EAT_FS_NO_ERROR == fh_read)
            {
                eat_fs_Close(fh_open);
                LOG_INFO("read_result :%s",buf);
                /*whatever the end of the line is ,eat_fs_Seek read the whole file*/
            }
            else
            {
                LOG_INFO("read log_file error:%d",fh_read);
            }
        }
        else
        {
            LOG_INFO("Seek File Pointer Fail");
        }
        eat_fs_Close(fh_open);
        return;

    }
    else
    {
        LOG_ERROR("open file failed, fh=%d!", fh_open);
        eat_fs_Close(fh_open);
        return;
    }
}



