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
    float voltage[40];
    float dump_mileage[40];

}DumpVoltage;

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
        u32 watchdog_timer_period;
        u32 at_cmd_timer_period;
        u32 gps_send_timer_period;
        u32 vibration_timer_period;
        u32 seek_timer_period;
        u32 socket_timer_period;
        u32 heartbeat_timer_period;
        u32 seekautooff_timer_peroid;
        u32 timeupdate_timer_peroid;
    };

    //Switch configuration
    eat_bool isVibrateFixed;
    DumpVoltage dump_voltage;
}SETTING;





extern SETTING setting;

eat_bool vibration_fixed(void) __attribute__((always_inline));
void set_vibration_state(eat_bool fixed) __attribute__((always_inline));


eat_bool setting_restore(void);

eat_bool updatertctime(void);
void setting_dump_voltage_init(void);


#define SETITINGFILE_NAME  L"C:\\setting.txt"

#define MILEAGEFILE_NAME   L"C:\\mileage.txt"
#define YEAROFFSET 1950

/*                             60V电源 100k&3k分压 没有初始里程数据                        */
#define mileage_initial {{1.747600,1.735175,1.722750,1.710325,1.697900,\
                          1.685475,1.673050,1.660625,1.648200,1.635775,\
                          1.623350,1.610925,1.598500,1.586075,1.573650,\
                          1.561225,1.548800,1.536375,1.523950,1.511525,\
                          1.499100,1.486675,1.474250,1.461825,1.449400,\
                          1.436975,1.424550,1.412125,1.399700,1.387275,\
                          1.374850,1.362425,1.350000,1.337575,1.325150,\
                          1.312725,1.300300,1.287875,1.275450,1.263025\
                          },\
                          {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,\
                           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\
                         }}




#endif /* USER_ELECTROMBILE_SETTING_H_ */
