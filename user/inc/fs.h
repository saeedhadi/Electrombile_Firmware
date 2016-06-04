/*
 * fs.h
 *
 *  Created on: 2016年2月19日
 *      Author: jk
 */

#ifndef USER_INC_FS_H_
#define USER_INC_FS_H_


#define SETTINGFILE_NAME  L"C:\\setting.conf"
#define ITINERARYFILE_NAME L"C:\\itinerary"

void fs_initial(void);
SINT64 fs_getDiskFreeSize(void);

int fs_factory(void);



#endif /* USER_INC_FS_H_ */
