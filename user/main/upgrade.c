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

int upgrade_appendFile(int offset, char* data,  unsigned int length)
{
    return 0;
}

int upgrade_do()
{
    return 0;
}
