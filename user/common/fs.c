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

int fs_cat_SettingFile(void)
{
    FS_HANDLE fh;
    eat_bool ret = EAT_FALSE;
    int rc;
    STORAGE storage;

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
            print("server domain = %s:%d.", storage.addr.domain, storage.port);
        }
        else if(ADDR_TYPE_IP == storage.addr_type)
        {
            print("server ip = %d.%d.%d.%d:%d.", storage.addr.ipaddr[0], storage.addr.ipaddr[1], storage.addr.ipaddr[2], storage.addr.ipaddr[3], storage.port);
        }
        ret = EAT_TRUE;
    }
    else
    {
        LOG_ERROR("read setting file fail, and return error: %d", fh);
    }

    eat_fs_Close(fh);

    return ret;
}

int fs_cat_MileageFile(void)
{
    FS_HANDLE fh;
    eat_bool ret = EAT_FALSE;
    int rc,i;
    DumpVoltage storage;

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

    rc = eat_fs_Read(fh, &storage, sizeof(STORAGE), NULL);
    if (EAT_FS_NO_ERROR == rc)
    {
        LOG_DEBUG("read mileage file success.");
        for(i = 0;i < MAX_MILEAGE_LEN;i++)
        {
            print("%d\tvoltage:%f\tmileage:%f\t\n",i+1,storage.voltage,storage.dump_mileage);
            eat_sleep(20);//the number is certain , 20ms is enough
        }

    }
    else
    {
        LOG_ERROR("read mileage file fail, and return error: %d", fh);
    }

    eat_fs_Close(fh);

    return rc;
}

int fs_cat_LogFile(void)
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
        print("log file does not exist.");
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
            uart_setWrite(fs_cat_LogFile);
        }


        if (!uart_buffer_full && end_of_file)
        {

            file_offset = 0;
            uart_setWrite(0);
        }

    }while (!uart_buffer_full && !end_of_file);

    eat_fs_Close(fh);

    return 0;
}



/*
 * cmd format: cat file.txt
 * must have a parameter, do not support wildcard
 */
static int fs_cat(const unsigned char* cmdString, unsigned short length)
{
    unsigned char* p = NULL;
    int rc;

    p = tool_StrstrAndReturnEndPoint((char *)cmdString, "mileage");
    if(NULL != p)
    {
        LOG_DEBUG("cat mileage");
        rc = fs_cat_MileageFile();
    }

    p = tool_StrstrAndReturnEndPoint((char *)cmdString, "setting");
    if(NULL != p)
    {
        LOG_DEBUG("cat setting");
        rc = fs_cat_SettingFile();
    }

    p = tool_StrstrAndReturnEndPoint((char *)cmdString, "log");
    if(NULL != p)
    {
        LOG_DEBUG("cat log.txt");
        rc = fs_cat_LogFile();
    }

    return rc;
}


void fs_initial(void)
{
    regist_cmd("ls", fs_ls);
    regist_cmd("rm", fs_rm);
    regist_cmd("cat", fs_cat);
}
