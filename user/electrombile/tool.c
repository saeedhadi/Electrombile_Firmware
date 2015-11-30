/*
 * tool.c
 *
 *  Created on: 2015Äê10ÔÂ16ÈÕ
 *      Author: jk
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <eat_interface.h>
#include <eat_modem.h>
#include <eat_type.h>

#include "tool.h"
#include "log.h"

eat_bool tool_modem_write(const unsigned char *data)
{
    unsigned short len = strlen(data);
    unsigned short ret = eat_modem_write(data, len);

    if(ret != len)
    {
        return EAT_FALSE;
    }
    else
    {
        return EAT_TRUE;
    }
}

unsigned char *tool_StrstrAndReturnEndPoint(char *str1, const char *str2)
{
    unsigned char *p;

    //LOG_DEBUG("GCY1:%s.", str1);
    //LOG_DEBUG("GCY2:%s.", str2);

    p = (unsigned char *)strstr(str1, str2);
    if(NULL != p)
    {
        p = p + strlen(str2);
    }

    return p;
}


