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

	//Timer configuration
    struct
    {
        u32 main_loop_timer_period;
        u32 vibration_timer_period;
        u32 seek_timer_period;
        u32 timeupdate_timer_peroid;
    };

    //Switch configuration
    eat_bool isVibrateFixed;


}SETTING;



extern SETTING setting;

eat_bool vibration_fixed(void) __attribute__((always_inline));
void set_vibration_state(eat_bool fixed) __attribute__((always_inline));

eat_bool setting_restore(void);
eat_bool setting_save(void);

#endif /* USER_ELECTROMBILE_SETTING_H_ */
