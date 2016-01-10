/*
 * data.h
 *
 *  Created on: 2015��7��9��
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_DATA_H_
#define USER_ELECTROMBILE_DATA_H_

#include <eat_type.h>

typedef struct
{
    eat_bool connected;     //is the socket connected to server
    eat_bool logined;       //is the client logined to server

    eat_bool isSeekMode;
    eat_bool isAutodefendFixed;
    unsigned char AutodefendPeriod;
}LOCAL_DATA;

eat_bool socket_conneted(void);
eat_bool client_logined(void);
eat_bool isSeekMode(void);

void set_socket_state(eat_bool connected);
void set_client_state(eat_bool logined);

//true means is in seek mode
void setSeekMode(eat_bool);

void set_autodefend_state(eat_bool fixed);
void set_autodefend_period(unsigned char period);
unsigned char get_autodefend_period(void);
eat_bool get_autodefend_state(void);



#endif /* USER_ELECTROMBILE_DATA_H_ */
