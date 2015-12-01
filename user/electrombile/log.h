/*
 * log.h
 *
 *  Created on: 2015/6/25
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_LOG_H_
#define USER_ELECTROMBILE_LOG_H_

#ifdef LOG_DEBUG_SERIAL
#define LOG_DEBUG(fmt, ...) eat_trace("[DBG][%s:%d %s]"fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#elif defined(LOG_DEBUG_REMOTE)
#define LOG_DEBUG(fmt, ...) log_remote("[%s:%d %s]"fmt,  __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

#ifdef LOG_INFO_SERIAL
#define LOG_INFO(fmt, ...) eat_trace("[INF][%s:%d %s]"fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#elif defined(LOG_INFO_REMOTE)
#define LOG_INFO(fmt, ...) log_remote("[%s:%d %s]"fmt,  __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...)
#endif

#ifdef LOG_ERROR_SERIAL
#define LOG_ERROR(fmt, ...) eat_trace("[ERR][%s:%d %s]"fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#elif defined(LOG_ERROR_REMOTE)
#define LOG_ERROR(fmt, ...) log_remote("[%s:%d %s]"fmt,  __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...)
#endif

#ifdef LOG_DEBUG_SERIAL
#define LOG_HEX(data, len)  log_hex(data, len);
#else
#define LOG_HEX(data, len)
#endif

void log_hex(const char* data, int length);
void log_remote(const char* fmt, ...);


#endif /* USER_ELECTROMBILE_LOG_H_ */
