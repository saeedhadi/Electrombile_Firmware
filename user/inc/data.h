/*
 * data.h
 *
 *  Created on: 2015??7??9??
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_DATA_H_
#define USER_ELECTROMBILE_DATA_H_

#include <eat_type.h>
#include "protocol.h"
#include "thread_msg.h"

typedef enum
{
	ITINERARY_START,
	ITINERARY_END
}ITINERARY_STATE;

eat_bool gps_isQueueFull(void);
eat_bool gps_isQueueEmpty(void);


eat_bool gps_enqueue(GPS* gps);
eat_bool gps_dequeue(GPS* gps);

int gps_size(void);

LOCAL_GPS* gps_get_last(void);
int gps_save_last(LOCAL_GPS* gps);

char get_itinerary_state(void);
void set_itinerary_state(char state);

int getVibrationTime(void);
int VibrationTimeAdd(void);
int ResetVibrationTime(void);

void set_manager_ATcmd_state(char state);
eat_bool get_manager_ATcmd_state(void);

void set_manager_seq(int seq);
int get_manager_seq(void);

void Vibration_setMoved(eat_bool state);
eat_bool Vibration_isMoved(void);

void Reset_AlarmCount(void);
int Get_AlarmCount(void);
void Add_AlarmCount(void);


#define MAX_GPS_COUNT 10
#define MAX_VLOTAGE_NUM 10


#endif /* USER_ELECTROMBILE_DATA_H_ */
