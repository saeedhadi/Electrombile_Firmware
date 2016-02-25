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
#include "setting.h"
#include "uart.h"
#include "log.h"

#define SYSTEM_DRIVE    "C:\\"
#define TF_DRIVE        "D:\\"

// because #include "mileage.h" can't pass compile , integrate after combine develop
#define MAX_MILEAGE_LEN 60
typedef struct
{
    float voltage[MAX_MILEAGE_LEN];
    float dump_mileage[MAX_MILEAGE_LEN];

}DumpVoltage;




//equivalent to eat_acsii_to_ucs2
static void ascii_2_unicode(u16* out, u8* in)
{
    u16 i=0;
    u8* outp = (u8*)out;
    u8* inp = in;
    //eat_trace("filename:%s",in);
    while( inp[i] )
    {
        outp[i*2] = inp[i];
        outp[i*2+1] = 0x00;
        i++;
    }

}

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
            print("%s\t %d\t %s\t %d-%d-%d %d:%d:%d\r\n",
                    fileinfo.FileName,
                    fileinfo.FileSize,
                    fileinfo.Attributes & FS_ATTR_DIR ? "Dir" : "File",
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
int fs_delete_file(const WCHAR * FileName)
{

    eat_fs_error_enum fs_Op_ret;

    fs_Op_ret = (eat_fs_error_enum)eat_fs_Delete(FileName);
    if(EAT_FS_NO_ERROR != fs_Op_ret && EAT_FS_FILE_NOT_FOUND != fs_Op_ret)
    {
        LOG_ERROR("Delete file Fail,and Return Error is %d",fs_Op_ret);
        return -1;
    }
    else
    {
        LOG_DEBUG("Delete file Success");
        return 0;
    }
}

/*
 * cmd format: rm file.txt
 * must have a parameter, do not support wildcard
 */
static int fs_rm(const unsigned char* cmdString, unsigned short length)
{
    unsigned char* p = NULL;
    int rc;

    p = tool_StrstrAndReturnEndPoint((char *)cmdString, "mileage");
    if(NULL != p)
    {
        LOG_DEBUG("delete mileage");
        rc = fs_delete_file(MILEAGEFILE_NAME);
    }

    p = tool_StrstrAndReturnEndPoint((char *)cmdString, "setting");
    if(NULL != p)
    {
        LOG_DEBUG("delete setting");
        rc = fs_delete_file(SETTINGFILE_NAME);
    }

    p = tool_StrstrAndReturnEndPoint((char *)cmdString, "log");
    if(NULL != p)
    {
        LOG_DEBUG("delete log.txt");
        rc = fs_delete_file(LOGFILE_NAME);
    }

    return rc;
}

/*
 * cmd format: cat file.txt
 * must have a parameter, do not support wildcard
 */
static int fs_cat(const unsigned char* cmdString, unsigned short length)
{
    unsigned char* p = NULL;
    int rc;

    return rc;
}


void fs_initial(void)
{
    regist_cmd("ls", fs_ls);
    regist_cmd("rm", fs_rm);
    regist_cmd("cat", fs_cat);
}
