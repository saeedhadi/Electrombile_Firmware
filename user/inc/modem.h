/*
 * modem.h
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#ifndef USER_INC_MODEM_H_
#define USER_INC_MODEM_H_


eat_bool modem_IsCallReady(char* modem_rsp);

#if 0
eat_bool modem_ReadGPRSStatus(void);
eat_bool modem_IsGPRSAttached(char* modem_rsp);
#else
eat_bool modem_GPRSAttach(void);
#endif

#endif /* USER_INC_MODEM_H_ */
