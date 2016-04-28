/*
 * msg_queue.h
 *
 *  Created on: 2016年4月20日
 *      Author: jk
 */

#ifndef USER_MAIN_MSG_QUEUE_H_
#define USER_MAIN_MSG_QUEUE_H_

#include "socket.h"

void msg_push(void* data, int len, MSG_RESEND_FAILED_HANDLER pfn, void* userdata);
void msg_resend(void);
void msg_ack(const void* msg);

void msg_startResend(void);
void msg_stopResend(void);

#endif /* USER_MAIN_MSG_QUEUE_H_ */
