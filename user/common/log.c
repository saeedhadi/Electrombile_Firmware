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
#include "debug.h"
#include "log.h"


int log_catlog(void)
{
#define READ_BUFFER_LENGTH  512
    FS_HANDLE fh;
    int rc = 0;
    char buf[READ_BUFFER_LENGTH] = {0};
    UINT readLen = 0;
    int printlen = 0;

    eat_bool uart_buffer_full = EAT_FALSE;
    eat_bool end_of_file = EAT_FALSE;

    static int file_offset = 0;

    fh = eat_fs_Open(LOGFILE_NAME, FS_READ_ONLY);

    //the log file is not found
    if(EAT_FS_FILE_NOT_FOUND == fh)
    {
        /*log_file == LOG_ERROR,there should not be LOG_ERROR*/
        print("log file not exists.");
        file_offset = 0;
        uart_setWrite(0);
        return -1;
    }

    if (fh < EAT_FS_NO_ERROR)
    {
        /*log_file == LOG_ERROR,there should not be LOG_ERROR*/
        print("open file failed, eat_fs_Open return %d!", fh);
        file_offset = 0;
        uart_setWrite(0);
        return -1;
    }

    rc = eat_fs_Seek(fh, file_offset, EAT_FS_FILE_BEGIN);
    if (rc < EAT_FS_NO_ERROR)
    {
        print("seek file pointer failed:%d", rc);
        eat_fs_Close(fh);
        file_offset = 0;
        uart_setWrite(0);
        return -1;
    }

    do
    {
        rc = eat_fs_Read(fh, buf, READ_BUFFER_LENGTH, &readLen);
        if (rc < EAT_FS_NO_ERROR)
        {
            print("read file failed:%d", rc);
            file_offset = 0;
            uart_setWrite(0);
            eat_fs_Close(fh);

            return -1;
        }

        if (readLen < READ_BUFFER_LENGTH)   //read the end of file
        {
            end_of_file = EAT_TRUE;
        }

        printlen = print("%s", buf);
        file_offset += printlen;
        if (printlen < readLen) //UART driver's receive buffer is full
        {
            uart_buffer_full = EAT_TRUE;
            uart_setWrite(log_catlog);
        }

//        LOG_INFO("read %d bytes, print %d bytes", readLen, printlen);

        if (!uart_buffer_full && end_of_file)
        {

            file_offset = 0;
            uart_setWrite(0);
        }

    }while (!uart_buffer_full && !end_of_file);

    eat_fs_Close(fh);

    return 0;
}

int cmd_catlog(const unsigned char* cmdString, unsigned short length)
{
    print("cat log file begin:");
    return log_catlog();
}

int cmd_deletelog(const unsigned char* cmdString, unsigned short length)
{
    eat_fs_error_enum fs_Op_ret;

    fs_Op_ret = (eat_fs_error_enum)eat_fs_Delete(LOGFILE_NAME);
    if(EAT_FS_NO_ERROR != fs_Op_ret && EAT_FS_FILE_NOT_FOUND != fs_Op_ret)
    {
        LOG_ERROR("Delete logfile Fail,and Return Error is %d",fs_Op_ret);
        return EAT_FALSE;
    }
    else
    {
        LOG_DEBUG("Delete logfile Success");
    }
    return EAT_TRUE;
}


void log_initial(void)
{
    regist_cmd("catlog", cmd_catlog);
    regist_cmd("deletelog", cmd_deletelog);
}

/*
 * The hex log is in the following format:
 *
 *     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F      0123456789ABCDEF
 * 01  aa 55 01 00 00 00 25 00 38 36 35 30 36 37 30 32     .U....%.86506702
 * 02  30 34 39 30 31 36 38 30 00 00 00 00 00 00 00 00     04901680........
 * 03  00 00 00 00 00 00 00 00 00 00 00 00                 ............
 *
 */
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

    fh_open = eat_fs_Open(LOGFILE_NAME, FS_READ_WRITE | FS_CREATE);

    if(EAT_FS_NO_ERROR <= fh_open)
    {
        LOG_INFO("open log_file success, fh = %d", fh_open);

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





