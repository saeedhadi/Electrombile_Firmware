/*
 * fs.c
 *
 *  Created on: 2016年2月19日
 *      Author: jk
 */


#include <string.h>

#include <eat_fs.h>

#include "fs.h"
#include "tool.h"
#include "uart.h"
#include "log.h"
#include "debug.h"
#include "utils.h"

#define SYSTEM_DRIVE    "C:\\"
#define TF_DRIVE        "D:\\"

#define CMD_STRING_LS   "ls"
#define CMD_STRING_RM   "rm"
#define CMD_STRING_CAT  "cat"



#define MAX_FILENAME_LEN    32
static int fs_ls(const unsigned char* cmdString, unsigned short length)
{
    FS_HANDLE fh;
    EAT_FS_DOSDirEntry fileinfo;
    WCHAR filename[MAX_FILENAME_LEN];


    fh = eat_fs_FindFirst(L"C:\\*.*", 0, 0, &fileinfo, filename, sizeof(filename));

    if (fh > 0)
    {

        do
        {
            //filename, file size, file attr, date
            print("%s\t %d\t %c%c%c%c%c\t %d-%d-%d %d:%d:%d\r\n",
                    fileinfo.FileName,
                    fileinfo.FileSize,
                    fileinfo.Attributes & FS_ATTR_DIR ? 'D' : '-',
                    fileinfo.Attributes & FS_ATTR_READ_ONLY ? 'R' : '-',
                    fileinfo.Attributes & FS_ATTR_HIDDEN ? 'H' : '-',
                    fileinfo.Attributes & FS_ATTR_SYSTEM ? 'S' : '-',
                    fileinfo.Attributes & FS_ATTR_ARCHIVE ? 'A' : '-',
                    fileinfo.CreateDateTime.Year1980 + 1980,
                    fileinfo.CreateDateTime.Month,
                    fileinfo.CreateDateTime.Day,
                    fileinfo.CreateDateTime.Hour,
                    fileinfo.CreateDateTime.Minute,
                    fileinfo.CreateDateTime.Second2);
        }while (eat_fs_FindNext(fh, &fileinfo, filename, sizeof(filename)) == EAT_FS_NO_ERROR);
    }

    eat_fs_FindClose(fh);

    return 0;
}


/*
 * cmd format: rm file.txt
 * must have a parameter, do not support wildcard
 */
static int fs_rm(const unsigned char* cmdString, unsigned short length)
{
    const unsigned char* filename = strstr(cmdString, CMD_STRING_RM);
    int rc = EAT_FS_NO_ERROR;
    WCHAR filename_w[MAX_FILENAME_LEN];


    filename = string_trimLeft(filename);
    if (strlen(filename) == 0)
    {
        LOG_INFO("parameter not correct");
        return 0;
    }

    ascii_2_unicode(filename_w, filename);      //FIXME: overflow bug: the filename length may exceed MAX_FILENAME_LEN

    rc = eat_fs_Delete(filename_w);
    if (rc == EAT_FS_FILE_NOT_FOUND)
    {
        LOG_INFO("file not found");
    }
    else if(rc == EAT_FS_NO_ERROR)
    {
        LOG_DEBUG("delete file Success");
    }
    else
    {
        LOG_ERROR("delete file fail, return code is %d", rc);
    }

    return rc;
}

/*
 * cmd format: cat file.txt
 * must have a parameter, do not support wildcard
 */
static int fs_cat(const unsigned char* cmdString, unsigned short length)
{
    int rc = 0;

    return rc;
}


void fs_initial(void)
{
    regist_cmd(CMD_STRING_LS, fs_ls);
    regist_cmd(CMD_STRING_RM, fs_rm);
    regist_cmd(CMD_STRING_CAT, fs_cat);
}

SINT64 fs_getDiskFreeSize(void)
{
    SINT64 size = 0;

    int rc = eat_fs_GetDiskFreeSize(EAT_FS, &size);
    if(rc == EAT_FS_NO_ERROR)
    {
        LOG_DEBUG("Get free disk size success,and the free disk size is %lld", size);
    }
    else
    {
        LOG_ERROR("Get free disk size failed, rc = %d",rc);
        return -1;
    }

    return size;
}
