/*
 * udp.c
 *
 *  Created on: 2016/6/18/
 *      Author: lc
 */
#ifndef UDP_CLIENT_H_
#define UDP_CLIENT_H_


int udp_socket_connect(u8 ip_addr[4]);

int udp_socket_sendData(void* data, s32 len);



#endif/*UDP_CLIENT_H_*/


