/*
 * log.h
 *
 *  Created on: 2015/6/25
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_LOG_H_
#define USER_ELECTROMBILE_LOG_H_

#include <eat_interface.h>

#ifdef APP_DEBUG
#define LOG_DEBUG(fmt, ...) eat_trace("[%d][DBG][%s:%d %s]"fmt, eat_get_task_id(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) eat_trace("[%d][INF][%s:%d %s]"fmt, eat_get_task_id(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) eat_trace("[%d][ERR][%s:%d %s]"fmt, eat_get_task_id(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__) ;\
                            log_file("[%s:%d %s]"fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define LOG_HEX(data, len)  log_hex(data, len);
#else
#define LOG_DEBUG(fmt, ...)
#define LOG_INFO(fmt, ...)
#define LOG_ERROR(fmt, ...)  log_file("[%s:%d %s]"fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define LOG_HEX(data, len)
#endif

void log_initial(void);

void log_hex(const char* data, int length);
void log_file(const char* fmt, ...)__attribute__((format(printf, 1, 2)));

#define LOG_FILE_NAME L"log.txt"
#define LOG_FILE_BAK L"log.old"

#define MAX_LOGFILE_SIZE (4 * 1024)

char* log_GetLog(void);


#endif /* USER_ELECTROMBILE_LOG_H_ */

