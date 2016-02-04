/*
 * uart.c
 *
 *  Created on: 2015��7��8��
 *      Author: jk
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <eat_interface.h>
#include <eat_uart.h>
#include <eat_modem.h>

#include "uart.h"
#include "debug.h"
#include "log.h"


int event_uart_ready_rd(const EatEvent_st* event)
{
	u16 length = 0;
	EatUart_enum uart = event->data.uart.uart;
	unsigned char buf[1024] = {0};	//TODO: use macro

	length = eat_uart_read(uart, buf, 1024);
	if (length)
	{
		buf[length] = 0;
		LOG_DEBUG("uart recv: %s", buf);
	}
	else
	{
		return 0;
	}

	debug_proc(buf, length);

	return 0;
}


void print(const char* fmt, ...)
{
    char buf[1024] = {0};
    int length = 0;

    va_list arg;
    va_start (arg, fmt);
    vsnprintf(buf, 1024, fmt, arg);
    va_end (arg);

    length = strlen(buf);

    eat_uart_write(EAT_UART_1, buf, length);
}
