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

#define LOGFILE_NAME  L"C:\\log_file.txt"

//TODO: this function need to be refactored
int cmd_catlog(const char* cmdString, unsigned short length)
{
    FS_HANDLE fh_open,fh_read;
    char buf[512] = "\0";
    char temp_buf[1024] = "\0";
    int seekRet = NULL;
    UINT readLen = 0;
    unsigned char *buf_p = NULL;
    eat_bool end_file = EAT_FALSE;

    fh_open = eat_fs_Open(LOGFILE_NAME, FS_READ_ONLY);

    if(EAT_FS_FILE_NOT_FOUND == fh_open)
    {
        /*log_file == LOG_ERROR,there should not be LOG_ERROR*/
        LOG_INFO("log file not exists.");
        return -1;
    }
    else if(EAT_FS_NO_ERROR <= fh_open)
    {
        LOG_INFO("open log_file success, fh=%d.", fh_open);

        seekRet = eat_fs_Seek(fh_open,0,EAT_FS_FILE_BEGIN);

        if(0 <= seekRet)
        {
            LOG_INFO("Seek File Pointer Success");
            while(EAT_TRUE)
            {
                eat_sleep(1);
                fh_read = eat_fs_Read(fh_open,&buf,512, &readLen);

                if(EAT_FS_NO_ERROR == fh_read && readLen == 512)
                {
                    //eat_fs_Close(fh_open);
                    strcpy(temp_buf+strlen(temp_buf),buf);
                    while(EAT_TRUE)
                    {
                        buf_p = strstr(temp_buf,"\r\n");
                        if(NULL == buf_p)
                        {
                            break;
                        }
                        strcpy(buf,buf_p+2);
                        memset(buf_p,0,strlen(buf_p));

                        LOG_INFO("%s",temp_buf);//print the line

                        eat_sleep(1);
                        strcpy(temp_buf,buf);
                    }

                }
                else if(EAT_FS_NO_ERROR == fh_read && readLen < 512)
                {
                    //eat_fs_Close(fh_open);
                    strcpy(temp_buf+strlen(temp_buf),buf);
                    while(EAT_TRUE)
                    {
                        buf_p = strstr(temp_buf,"\r\n");
                        if(NULL == buf_p)
                        {
                            end_file = EAT_TRUE;
                            break;
                        }
                        strcpy(buf,buf_p+2);
                        memset(buf_p,0,strlen(buf_p));

                        LOG_INFO("%s",temp_buf);//print the line

                        eat_sleep(1);
                        strcpy(temp_buf,buf);
                    }
                    if(EAT_TRUE == end_file)
                        break;
                }
                else
                {
                    /*log_file == LOG_ERROR,there should not be LOG_ERROR*/
                    LOG_INFO("read log_file error:%d at %d",fh_read,readLen);
                    break;
                }
            }
        }
        else
        {
            /*log_file == LOG_ERROR,there should not be LOG_ERROR*/
            LOG_INFO("Seek File Pointer Fail");
        }
        eat_fs_Close(fh_open);
        return 0;
    }
    else
    {
        /*log_file == LOG_ERROR,there should not be LOG_ERROR*/
        LOG_INFO("open file failed, fh=%d!", fh_open);
        eat_fs_Close(fh_open);
        return -1;
    }
}

void log_initial(void)
{
    regist_cmd("catlog", cmd_catlog);
}

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
            if (data[j] < 32 || data[j] >= 127)
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


    strcpy(buf+strlen(buf),"\r\n");

    fh_open = eat_fs_Open(LOGFILE_NAME, FS_READ_WRITE|FS_CREATE);

    if(EAT_FS_NO_ERROR <= fh_open)
    {
        LOG_INFO("create or open log_file success, fh=%d.", fh_open);

        seekRet = eat_fs_Seek(fh_open,0,EAT_FS_FILE_END);

        if(seekRet < 0)
        {
            /*log_file == LOG_ERROR,there should not be LOG_ERROR*/
            LOG_INFO("Seek File Pointer Fail");

            eat_fs_Close(fh_open);

            return;
        }
        else
        {
            LOG_INFO("Seek File Pointer Success");

            fh_write = eat_fs_Write(fh_open, buf, strlen(buf), &writedLen);
            LOG_DEBUG("%s",buf);
            if((EAT_FS_NO_ERROR == fh_write) && (strlen(buf) == writedLen))
            {

                fh_commit = eat_fs_Commit(fh_open);
                if(EAT_FS_NO_ERROR == fh_commit)
                {
                    LOG_DEBUG("commit file success.");
                }
                else
                {
                    /*log_file == LOG_ERROR,there should not be LOG_ERROR*/
                    LOG_INFO("commit file failed, and Return Error is %d.", fh_commit);
                }
            }
            else
            {
                /*log_file == LOG_ERROR,there should not be LOG_ERROR*/
                LOG_INFO("write file failed,Error is%d, writedLen is%d,strlen(buf) is%d",fh_write,writedLen,strlen(buf));
            }
            eat_fs_Close(fh_open);
            return;
        }

    }
    else
    {
        /*log_file == LOG_ERROR,there should not be LOG_ERROR*/
        LOG_INFO("open file failed, fh=%d!", fh_open);
        eat_fs_Close(fh_open);
        return;
    }

}

