/*
 * uart.h
 *
 *  Created on: 2015/7/8
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_UART_H_
#define USER_ELECTROMBILE_UART_H_

#include <eat_interface.h>

typedef int (*CMD_ACTION)(const char* cmdString, unsigned short length);
int regist_cmd(const char* cmd, CMD_ACTION action);

int event_uart_ready_rd(const EatEvent_st* event);

void print(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

#endif /* USER_ELECTROMBILE_UART_H_ */
