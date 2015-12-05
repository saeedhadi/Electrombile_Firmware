/*
 * data.h
 *
 *  Created on: 2015Äê7ÔÂ9ÈÕ
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_DATA_H_
#define USER_ELECTROMBILE_DATA_H_

#include <eat_type.h>

#include "protocol.h"

typedef struct
{
    eat_bool connected;     //is the socket connected to server
    eat_bool logined;       //is the client logined to server

    eat_bool isSeekFixed;
    eat_bool isAutodefendFixed;
    unsigned char AutodefendPeriod;

    eat_bool isGpsFixed;
    eat_bool isCellGet;
    GPS gps;
    CGI cgi;
    CELL cells[MAX_CELL_NUM];
}LOCAL_DATA;

extern LOCAL_DATA data;


eat_bool socket_conneted(void);
eat_bool client_logined(void);
eat_bool seek_fixed(void);
eat_bool get_autodefend_state(void);
unsigned char get_autodefend_period(void);

void set_socket_state(eat_bool connected);
void set_client_state(eat_bool logined);
void set_seek_state(eat_bool fixed);
void set_autodefend_state(eat_bool fixed);
void set_autodefend_period(unsigned char period);

#endif /* USER_ELECTROMBILE_DATA_H_ */
