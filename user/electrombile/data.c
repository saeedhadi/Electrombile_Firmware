/*
 * data.c
 *
 *  Created on: 2015��7��9��
 *      Author: jk
 */


#include "data.h"


LOCAL_DATA data =
{
    EAT_FALSE,
    EAT_FALSE,
    EAT_FALSE,
    EAT_TRUE,
    5,
    EAT_FALSE,
    EAT_FALSE,
    {0}
};


eat_bool socket_conneted(void)
{
    return data.connected;
}

void set_socket_state(eat_bool connected)
{
    data.connected = connected;
}

eat_bool client_logined(void)
{
    return data.logined;
}

void set_client_state(eat_bool logined)
{
    data.logined = logined;
}

eat_bool isSeekMode(void)
{
    return data.isSeekMode;
}

void setSeekMode(eat_bool fixed)
{
    data.isSeekMode = fixed;
}

eat_bool get_autodefend_state(void)
{
    return data.isAutodefendFixed;
}

void set_autodefend_state(eat_bool fixed)
{
    data.isAutodefendFixed = fixed;
}

unsigned char get_autodefend_period(void)
{
    return data.AutodefendPeriod;
}

void set_autodefend_period(unsigned char period)
{
    data.AutodefendPeriod = period;
}


