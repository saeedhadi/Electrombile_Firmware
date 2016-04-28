/*
 * socket.h
 *
 *  Created on: 2015��7��8��
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_SOCKET_H_
#define USER_ELECTROMBILE_SOCKET_H_

/*
 * 消息发送失败处理函数，如:将消息持久化待下次发送
 * userdata为回调函数的附加参数，由调用方确定，如不需要可置为null
 */
typedef int (*MSG_RESEND_FAILED_HANDLER)(void* msg, int length, void* userdata);

int socket_init(void);
int socket_setup(void);

int socket_sendData(void* data, int len);

/*
 * 直接发送消息，无需重传
 */
int socket_sendDataDirectly(void* data, int len);

/*
 * 发送消息并等待ack，如果没有收到ack则重传
 */
int socket_sendDataWaitAck(void* data, int len, MSG_RESEND_FAILED_HANDLER pfn, void* userdata);


#endif /* USER_ELECTROMBILE_SOCKET_H_ */
