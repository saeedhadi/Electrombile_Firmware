/*
 * udp.c
 *
 *  Created on: 2016/6/18/
 *      Author: lc
 */

#ifndef UDP_CLIENT_H_
#define UDP_CLIENT_H_


int socket_connect_udp(void);
s32 socket_sendData_udp(void* data, s32 len);


#endif/*UDP_CLIENT_H_*/

