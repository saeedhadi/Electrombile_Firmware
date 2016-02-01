/*
 * client.h
 *
 *  Created on: 2015/7/9
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_CLIENT_H_
#define USER_ELECTROMBILE_CLIENT_H_

int client_proc(const void* m, int msgLen);
void client_loop(void);
void msg_heartbeat(void);


void send_autodefendstate_msg(eat_bool state);

void msg_wild(const void*, int);

#endif /* USER_ELECTROMBILE_CLIENT_H_ */
