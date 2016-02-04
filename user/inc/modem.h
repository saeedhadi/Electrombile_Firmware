/*
 * modem.h
 *
 *  Created on: 2016年2月4日
 *      Author: jk
 */

#ifndef USER_INC_MODEM_H_
#define USER_INC_MODEM_H_

eat_bool modem_ReadGPRSStatus(void);

eat_bool modem_IsCallReady(char* modem_rsp);

eat_bool modem_IsGPRSAttached(char* modem_rsp);


#endif /* USER_INC_MODEM_H_ */
