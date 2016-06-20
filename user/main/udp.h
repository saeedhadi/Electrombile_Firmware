/*
 * udp.c
 *
 *  Created on: 2016/6/18/
 *      Author: lc
 */
#ifndef UDP_CLIENT_H_
#define UDP_CLIENT_H_


int socket_connect_udp(u8 ip_addr[4]);

int socket_sendData_UDP(void* data, s32 len);



#endif/*UDP_CLIENT_H_*/


