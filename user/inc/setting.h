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



#pragma anon_unions
typedef struct
{
	//Server configuration
	ADDR_TYPE addr_type;
	union
	{
		char domain[MAX_DOMAIN_NAME_LEN];
		u8 ipaddr[4];
	};
	u16 port;
    u16 port_udp;

	//Timer configuration
    struct
    {
        u32 main_loop_timer_period;
        u32 vibration_timer_period;
        u32 seek_timer_period;
        u32 timeupdate_timer_peroid;
        u32 gps_send_period;
    };

    //Switch configuration
    eat_bool isVibrateFixed;

    //autolock configration
    struct
    {
        eat_bool isAutodefendFixed;
        unsigned char autodefendPeriod;
    };

    //battery type
    unsigned char BaterryType;


}SETTING;



extern SETTING setting;

eat_bool vibration_fixed(void) __attribute__((always_inline));
void set_vibration_state(eat_bool fixed) __attribute__((always_inline));

void set_autodefend_state(eat_bool fixed);
unsigned char get_autodefend_period(void);

void set_autodefend_period(unsigned char period);
eat_bool get_autodefend_state(void);

unsigned char get_battery_type(void);
void set_battery_type(unsigned char batterytype);


eat_bool setting_restore(void);
eat_bool setting_save(void);

#endif /* USER_ELECTROMBILE_SETTING_H_ */
