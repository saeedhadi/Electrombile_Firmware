/*
 * fs.h
 *
 *  Created on: 2016年2月19日
 *      Author: jk
 */

#ifndef USER_INC_FS_H_
#define USER_INC_FS_H_


#define MILEAGEFILE_NAME   L"C:\\mileage"
#define SETTINGFILE_NAME  L"C:\\setting"
#define LOGFILE_NAME  L"C:\\log_file.txt"

void fs_initial(void);
SINT64 fs_getDiskFreeSize(void);


#endif /* USER_INC_FS_H_ */
