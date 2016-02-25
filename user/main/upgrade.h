/*
 * upgrade.h
 *
 *  Created on: 2016年2月18日
 *      Author: jk
 */

#ifndef USER_MAIN_UPGRADE_H_
#define USER_MAIN_UPGRADE_H_

int upgrade_createFile(void);
int upgrade_appendFile(int offset, char* data, unsigned int length);
int upgrade_do(void);
u8* get_AppFile(int *filesize);
int upgrade_adler32(unsigned char *data, size_t len);



#endif /* USER_MAIN_UPGRADE_H_ */
