/*
 * vibration.h
 *
 *  Created on: 2015/7/1
 *      Author: jk
 */
#ifndef ELECTROMBILE_FIRMWARE_VIBRATION_H
#define ELECTROMBILE_FIRMWARE_VIBRATION_H
typedef enum
{
	ITINERARY_START,
	ITINERARY_END
}ITINERARY_STATE;

void app_vibration_thread(void *data);

#endif //ELECTROMBILE_FIRMWARE_VIBRATION_H
