/*
 * request.h
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#ifndef USER_MAIN_REQUEST_H_
#define USER_MAIN_REQUEST_H_

int cmd_Login(void);
int cmd_SMS(const void* msg);
void cmd_Heartbeat(void);
int cmd_Seek(unsigned int value);


#endif /* USER_MAIN_REQUEST_H_ */
