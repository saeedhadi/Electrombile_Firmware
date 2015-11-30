/*
 * data.c
 *
 *  Created on: 2015Äê7ÔÂ9ÈÕ
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

eat_bool seek_fixed(void)
{
    return data.isSeekFixed;
}

void set_seek_state(eat_bool fixed)
{
    data.isSeekFixed = fixed;
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


