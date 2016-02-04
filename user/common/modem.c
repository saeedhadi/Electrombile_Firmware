/*
 * modem.c
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#include <string.h>

#include <eat_interface.h>
#include <eat_modem.h>
#include <eat_type.h>

#include "modem.h"
#include "log.h"

#define MODEM_TEST_CMD  "=?"
#define MODEM_READ_CMD  "?"
#define MODEM_WRITE_CMD "="

#define NEALINE    "\n"


#define AT_CGATT    "AT+CGATT"

static eat_bool modem_cmd(const unsigned char *cmd)
{
    unsigned short len = strlen(cmd);
    unsigned short rc = eat_modem_write(cmd, len);

    if(rc != len)
    {
        LOG_ERROR("modem write failed: should be %d, but %d", len, rc);
        return EAT_FALSE;
    }
    else
    {
        return EAT_TRUE;
    }
}

eat_bool modem_ReadGPRSStatus()
{
    unsigned char* cmd = AT_CGATT MODEM_READ_CMD NEALINE;

    return modem_cmd(cmd);
}

eat_bool modem_IsGPRSAttached(char* modem_rsp)
{
    char* ptr = strstr((const char *) modem_rsp, "+CGATT: 1");

    if (ptr)
    {
        return EAT_TRUE;
    }

    return EAT_FALSE;
}

