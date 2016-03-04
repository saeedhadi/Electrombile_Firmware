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

#define CR  "\d"    //CR (carriage return)
#define LF  "\n"    //LF (line feed - new line)


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


eat_bool modem_IsCallReady(char* modem_rsp)
{
    char* ptr = strstr((const char *) modem_rsp, "Call Ready");

    if (ptr)
    {
        return EAT_TRUE;
    }

    return EAT_FALSE;
}

#if 0
eat_bool modem_ReadGPRSStatus(void)
{
    unsigned char* cmd = AT_CGATT MODEM_READ_CMD LF;


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
#else

eat_bool modem_GPRSAttach()
{
    int rc = eat_network_get_creg();

    if (rc == EAT_REG_STATE_REGISTERED)
    {
        return eat_network_get_cgatt();
    }
    else
    {
        LOG_DEBUG("network register status: %d", rc);
    }

    return EAT_FALSE;

}

#endif
