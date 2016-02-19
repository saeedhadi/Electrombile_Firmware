/*
 * fs.c
 *
 *  Created on: 2016年2月19日
 *      Author: jk
 */


#include <string.h>

#include <eat_fs.h>

#include "fs.h"

#define SYSTEM_DRIVE    "C:\\"
#define TF_DRIVE        "D:\\"


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
static int fs_ls(const unsigned char* cmdString, unsigned short length)
{
    return 0;
}

/*
 * cmd format: rm file.txt
 * must have a parameter, do not support wildcard
 */
static int fs_rm(const unsigned char* cmdString, unsigned short length)
{
    return 0;
}

/*
 * cmd format: cat file.txt
 * must have a parameter, do not support wildcard
 */
static int fs_cat(const unsigned char* cmdString, unsigned short length)
{
    return 0;
}


void fs_initial(void)
{
    regist_cmd("ls", fs_ls);
    regist_cmd("rm", fs_rm);
    regist_cmd("cat", fs_cat);
}
