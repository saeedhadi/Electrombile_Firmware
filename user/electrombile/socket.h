/*
 * socket.h
 *
 *  Created on: 2015��7��8��
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_SOCKET_H_
#define USER_ELECTROMBILE_SOCKET_H_


void socket_init(void);

s32 socket_sendData(const void* data, s32 len);

eat_bool socket_conneted();

#endif /* USER_ELECTROMBILE_SOCKET_H_ */