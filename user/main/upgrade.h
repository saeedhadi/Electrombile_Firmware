/*
 * upgrade.h
 *
 *  Created on: 2016年2月18日
 *      Author: jk
 */

#ifndef USER_MAIN_UPGRADE_H_
#define USER_MAIN_UPGRADE_H_

int upgrade_createFile();
int upgrade_appendFile(int offset, char* data, unsigned int length);
int upgrade_do();

#endif /* USER_MAIN_UPGRADE_H_ */
