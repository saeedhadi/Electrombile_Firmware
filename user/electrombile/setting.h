/*
 * setting.h
 *
 *  Created on: 2015/6/24
 *      Author: jk
 */

#ifndef USER_ELECTROMBILE_SETTING_H_
#define USER_ELECTROMBILE_SETTING_H_

#include <eat_type.h>

#define MAX_DOMAIN_NAME_LEN 32
typedef enum
{
	ADDR_TYPE_IP,
	ADDR_TYPE_DOMAIN
}ADDR_TYPE;

typedef struct
{
	//Server configuration
	ADDR_TYPE addr_type;
	union
	{
		s8 domain[MAX_DOMAIN_NAME_LEN];
		u8 ipaddr[4];
	}addr;
	u16 port;

	//Timer configuration
    u32 gps_send_timer_period;
}STORAGE;

typedef struct
{
	//Server configuration
	ADDR_TYPE addr_type;
	union
	{
		s8 domain[MAX_DOMAIN_NAME_LEN];
		u8 ipaddr[4];
	}addr;
	u16 port;

	//Timer configuration
	u32 watchdog_timer_period;
    u32 at_cmd_timer_period;
	u32 gps_timer_period;
    u32 gps_send_timer_period;
	u32 vibration_timer_period;
    u32 seek_timer_period;
    u32 socket_timer_period;

    //Switch configuration
    eat_bool isVibrateFixed;
}SETTING;

extern SETTING setting;

eat_bool vibration_fixed(void);
void set_vibration_state(eat_bool fixed);
eat_bool setting_initial(void);
void setting_reset(void);
eat_bool storage_check(void);
eat_bool storage_save(void);
void convert_storage_to_setting(void);
void convert_setting_to_storage(void);

#endif /* USER_ELECTROMBILE_SETTING_H_ */
