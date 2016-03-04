/*
 * log.h
 *
 *  Created on: 2015/6/25
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_LOG_H_
#define USER_ELECTROMBILE_LOG_H_

#include <eat_interface.h>

#ifdef LOG_DEBUG_FLAG
#define LOG_DEBUG(fmt, ...) eat_trace("[%d][DBG][%s:%d %s]"fmt, eat_get_task_id(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

#ifdef LOG_INFO_FLAG
#define LOG_INFO(fmt, ...) eat_trace("[%d][INF][%s:%d %s]"fmt, eat_get_task_id(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...)
#endif

#ifdef LOG_ERROR_FLAG
#define LOG_ERROR(fmt, ...) eat_trace("[%d][ERR][%s:%d %s]"fmt, eat_get_task_id(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__) ;\
                            log_file("[%s:%d %s]"fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...)
#endif

#ifdef LOG_DEBUG_FLAG
#define LOG_HEX(data, len)  log_hex(data, len);
#else
#define LOG_HEX(data, len)
#endif

#ifdef LOG_DEBUG_FLAG
#define LOG_REMOTE(fmt, ...) log_remote("[%s:%d %s]"fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define LOG_REMOTE(fmt, ...)
#endif

void log_initial(void);

void log_hex(const char* data, int length);
void log_file(const char* fmt, ...)__attribute__((format(printf, 1, 2)));

#define LOGFILE_NAME  L"C:\\log_file.txt"



#endif /* USER_ELECTROMBILE_LOG_H_ */
