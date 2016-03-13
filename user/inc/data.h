/*
 * data.h
 *
 *  Created on: 2015��7��9��
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_DATA_H_
#define USER_ELECTROMBILE_DATA_H_

#include <eat_type.h>
#include "protocol.h"

eat_bool gps_isQueueFull(void);
eat_bool gps_isQueueEmpty(void);

eat_bool gps_enqueue(GPS* gps);
eat_bool gps_dequeue(GPS* gps);

int gps_size(void);

#endif /* USER_ELECTROMBILE_DATA_H_ */
