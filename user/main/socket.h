/*
 * socket.h
 *
 *  Created on: 2015��7��8��
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_SOCKET_H_
#define USER_ELECTROMBILE_SOCKET_H_


int socket_init(void);
int socket_setup(void);
int socket_connect(void);
void socket_close(void);
s32 socket_sendData(void* data, s32 len);


#endif /* USER_ELECTROMBILE_SOCKET_H_ */
