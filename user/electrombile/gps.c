/*
 * gps.c
 *
 *  Created on: 2015/6/25
 *      Author: jk
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <eat_interface.h>
#include <eat_modem.h>

#include "gps.h"
#include "timer.h"
#include "thread.h"
#include "thread_msg.h"
#include "log.h"
#include "setting.h"
#include "protocol.h"
#include "tool.h"
#include "math.h"

static void gps_timer_handler(u8 cmd);
static void gps_at_read_handler(void);
static eat_bool gps_sendGps(u8 cmd);
static eat_bool gps_sendCell(u8 cmd);
static eat_bool gps_isGpsFixed(void);
static eat_bool gps_isCellGet(void);
static eat_bool gps_DuplicateCheck(LOCAL_GPS *pre_gps, LOCAL_GPS *gps);
static long double gps2rad(double d);
static long double getdistance(LOCAL_GPS *pre_gps, LOCAL_GPS *gps);

#define NMEA_BUFF_SIZE 1024
#define READ_BUFF_SIZE 2048
#define LATITUDE_THRESHOLD  0.001f
#define LONGITUDE_THRESHOLD 0.001f
#define EARTH_RADIUS 6378137 //radius of our earth unit :  m
#define pi 3.141592653


//static char gps_info_buf[READ_BUFF_SIZE]="";
static eat_bool isGpsFixed = EAT_FALSE;
static float latitude = 0.0;
static float longitude = 0.0;
static float altitude = 0.0;
static float speed = 0.0;
static float course = 0.0;



static eat_bool isCellGet = EAT_FALSE;
static short mcc = 0;//mobile country code
static short mnc = 0;//mobile network code
static char  cellNo = 0;//cell count
static CELL  cells[7] = {0};
static LOCAL_GPS last_gps_info;
static LOCAL_GPS* last_gps =&last_gps_info;//gps sent for the last time

void app_gps_thread(void *data)
{
    EatEvent_st event;
	MSG_THREAD* msg;
	u8 msgLen;

    LOG_INFO("gps thread start.");

    eat_timer_start(TIMER_GPS, setting.gps_timer_period);
    tool_modem_write("AT+CGNSPWR=1\r");//turn on GNSS power supply
    tool_modem_write("AT+CENG=3,1\r");//set cell on

    while(EAT_TRUE)
    {
        eat_get_event_for_user(THREAD_GPS, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER :
                switch (event.data.timer.timer_id)
                {
                    case TIMER_GPS:
                    	LOG_INFO("TIMER_GPS expire.");
                        gps_timer_handler(CMD_THREAD_GPS);
                        eat_timer_start(TIMER_GPS, setting.gps_timer_period);
                        break;

                    default:
                    	LOG_ERROR("ERR: timer[%d] expire!", event.data.timer.timer_id);
                        break;
                }
                break;

            case EAT_EVENT_MDM_READY_RD:
                LOG_DEBUG("gps AT read.");
                gps_at_read_handler();
                break;

            case EAT_EVENT_USER_MSG:
                msg = (MSG_THREAD*) event.data.user_msg.data_p;
                msgLen = event.data.user_msg.len;

                switch (msg->cmd)
                {
                    case CMD_THREAD_LOCATION:
                        LOG_DEBUG("gps get CMD_THREAD_LOCATION.");
                        gps_timer_handler(CMD_THREAD_LOCATION);
                        break;
                    default:
                        LOG_ERROR("cmd(%d) not processed!", msg->cmd);
                        break;
                }
                break;

            default:
            	LOG_ERROR("event(%d) not processed!", event.event);
                break;

        }
    }
}

static void gps_timer_handler(u8 cmd)
{
    if(gps_isGpsFixed())
    {
        LOG_DEBUG("send gps.");
        gps_sendGps(cmd);
    }
    else if(gps_isCellGet())
    {
        LOG_DEBUG("send cells.");
        gps_sendCell(cmd);
    }

    return;
}

static eat_bool gps_isGpsFixed(void)
{
    eat_bool ret = isGpsFixed;

    isGpsFixed = EAT_FALSE;
    tool_modem_write("AT+CGNSINF\r");

    return ret;
}

static eat_bool gps_isCellGet(void)
{
    eat_bool ret = isCellGet;

    isCellGet = EAT_FALSE;
    tool_modem_write("AT+CENG?\r");

    return ret;
}

static eat_bool gps_sendGps(u8 cmd)
{
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(LOCAL_GPS);
    MSG_THREAD* msg = allocMsg(msgLen);
    LOCAL_GPS* gps = 0;
    eat_bool cmp = EAT_FALSE;
    eat_bool ret = EAT_FALSE;

    if (!msg)
    {
        LOG_ERROR("alloc msg failed!");
        return EAT_FALSE;
    }
    msg->cmd = cmd;//CMD_THREAD_GPS or CMD_THREAD_LOCATION
    msg->length = sizeof(LOCAL_GPS);

    gps = (LOCAL_GPS*)msg->data;

    gps->isGpsFixed = EAT_TRUE;
    gps->gps.latitude = latitude;
    gps->gps.longitude = longitude;
    gps->gps.altitude = altitude;
    gps->gps.speed = speed;
    gps->gps.course = course;

    if(last_gps == 0 || msg->cmd == CMD_THREAD_LOCATION)
    {
        LOG_DEBUG("the first cell.");

        //last_gps = (LOCAL_GPS*)eat_mem_alloc(sizeof(LOCAL_GPS));

        cmp = EAT_FALSE;
    }
    else
    {
        cmp = gps_DuplicateCheck(last_gps, gps);
    }

    if(EAT_TRUE == cmp)
    {
        //GPS is the same as before, do not send this msg
        ret = EAT_TRUE;
    }

    else
    {
        memcpy(last_gps, gps, sizeof(LOCAL_GPS));

        //GPS is different from before, send this msg, update the last_gps
        LOG_DEBUG("send gps to THREAD_MAIN: latitude(%f), longitude(%f).", latitude, longitude);
        ret = sendMsg(THREAD_GPS, THREAD_MAIN, msg, msgLen);
    }

    /* can not freeMsg(msg) here, if do, it will be crashed. */

    return ret;
}

static eat_bool gps_sendCell(u8 cmd)
{
    u8 msgLen = sizeof(MSG_THREAD) + sizeof(LOCAL_GPS);
    MSG_THREAD *msg = allocMsg(msgLen);
    LOCAL_GPS *gps = 0;
    int i = 0;
    eat_bool cmp = EAT_FALSE;
    eat_bool ret = EAT_FALSE;

    if (!msg)
    {
        LOG_ERROR("alloc msg failed!");
        return EAT_FALSE;
    }
    msg->cmd = cmd;//CMD_THREAD_GPS or CMD_THREAD_LOCATION
    msg->length = sizeof(LOCAL_GPS);

    gps = (LOCAL_GPS*)msg->data;
    gps->isGpsFixed = EAT_FALSE;

    gps->cellInfo.mcc = mcc;
    gps->cellInfo.mnc = mnc;
    gps->cellInfo.cellNo = cellNo;

    for (i = 0; i < cellNo; i++)
    {
        gps->cellInfo.cell[i].lac = cells[i].lac;
        gps->cellInfo.cell[i].cellid = cells[i].cellid;
        gps->cellInfo.cell[i].rxl = cells[i].rxl;
    }

    if(last_gps == 0 || msg->cmd == CMD_THREAD_LOCATION)
    {
        LOG_DEBUG("the first cell or active acquisition");

        //last_gps = (LOCAL_GPS*)eat_mem_alloc(sizeof(LOCAL_GPS));

        cmp = EAT_FALSE;
    }
    else
    {
        cmp = gps_DuplicateCheck(last_gps, gps);
    }

    if(EAT_TRUE == cmp)
    {
        //GPS is the same as before, do not send this msg
        ret = EAT_TRUE;
    }
    else
    {
        memcpy(last_gps, gps, sizeof(LOCAL_GPS));

        //GPS is different from before, send this msg, update the last_gps
        LOG_DEBUG("send cell to THREAD_MAIN: mcc(%d), mnc(%d), cellNo(%d).", mcc, mnc, cellNo);
        ret = sendMsg(THREAD_GPS, THREAD_MAIN, msg, msgLen);
    }

    /* can not freeMsg(msg) here, if do, it will be crashed. */

    return ret;
}

static void gps_at_read_handler(void)
{
    unsigned char *buf_p1 = NULL;
    unsigned char *buf_p2 = NULL;
    unsigned char  buf[READ_BUFF_SIZE] = {0};  //用于读取AT指令的响应
    unsigned int len = 0;
    unsigned int count = 0, cellCount = 0;
    int _mcc = 0;
    int _mnc = 0;
    int lac = 0;
    int cellid = 0;
    int rxl = 0;

    len = eat_modem_read(buf, READ_BUFF_SIZE);
    LOG_DEBUG("modem read, len=%d, buf=\r\n%s", len, buf);

    buf_p1 = tool_StrstrAndReturnEndPoint(buf, "AT+CGNSPWR=1\r\r\n");
    if(NULL != buf_p1)
    {
        buf_p2 = (unsigned char*)strstr(buf_p1, "OK");
        if(buf_p1 == buf_p2)
        {
            LOG_DEBUG("turn on gps power OK.");
        }
    }

    buf_p1 = tool_StrstrAndReturnEndPoint(buf, "AT+CENG=3,1\r\r\n");
    if(NULL != buf_p1)
    {
        buf_p2 = (unsigned char*)strstr(buf_p1, "OK");
        if(buf_p1 == buf_p2)
        {
            LOG_DEBUG("turn on cells OK.");
        }
    }

    /*
     * NMEA output format:
     * +CGNSINF: 1,1,20150327014838.000,31.221783,121.354528,114.600,0.28,0.0,1,,1.9,2.2,1.0,,8,4,,,42,,
     * +CGNSINF: 1,0,20151016130202.000,,,,0.11,40.7,0,,,,,,11,1,,,37,,
     * <GNSS run status>,<Fix status>,<UTC date & Time>,<Latitude>,<Longitude>,<MSL Altitude>,<Speed Over Ground>,<Course Over Ground>,
     * <Fix Mode>,<Reserved1>,<HDOP>,<PDOP>,<VDOP>,<Reserved2>,<Satellites in View>,<Satellites Used>,<Reserved3>,<C/N0 max>,<HPA>,<VPA>
     */
    buf_p1 = tool_StrstrAndReturnEndPoint(buf, "+CGNSINF: ");
    if(NULL != buf_p1)
    {
        count = sscanf(buf_p1, "%*d,%d,%*f,%f,%f,%f,%f,%f,%*s",&isGpsFixed, &latitude, &longitude,&altitude,&speed,&course);
        if(6 != count)
        {
            LOG_DEBUG("gps not fixed.count = %d",count);
            isGpsFixed = EAT_FALSE;
        }
        else
        {
            LOG_DEBUG("gps fixed=%d:latitude=%f,longitude=%f,altitude=%f,speed=%f,course=%f", isGpsFixed, latitude, longitude,altitude,speed,course);
        }
    }

    /*
     * the output format of AT+CENG?
     * +CENG:<mode>,<Ncell>
     * +CENG:<cell>,"<mcc>,<mnc>,<lac>,<cellid>,<bsic>,<rxl>"
     * +CENG:<cell>,...
     *
     * eg:
     * +CENG: 3,1
     *
     * +CENG: 0,"460,00,5001,96bd,23,45"
     * +CENG: 1,"460,00,5001,96dd,33,21"
     * +CENG: 2,",,0000,0000,00,00"
     * +CENG: 3,",,0000,0000,00,00"
     * +CENG: 4,",,0000,0000,00,00"
     * +CENG: 5,",,0000,0000,00,00"
     * +CENG: 6,",,0000,0000,00,00"
     *
     */
    if(EAT_FALSE == isGpsFixed)
    {
        buf_p1 = tool_StrstrAndReturnEndPoint(buf, "+CENG: 3,1\r\n\r\n");
        if(NULL != buf_p1)
        {
            while(buf_p1)
            {
                buf_p1 = tool_StrstrAndReturnEndPoint(buf_p1, "+CENG: ");

                count = sscanf(buf_p1, "%d,\"%d,%d,%x,%x,%*d,%d\"", &cellCount, &_mcc, &_mnc, &lac, &cellid, &rxl);
                if(6 != count)
                {
                    continue;
                }

                if(0 == mcc)
                {
                    mcc = _mcc;
                    mnc = _mnc;
                }
                cells[cellCount].lac = lac;
                cells[cellCount].cellid = cellid & 0xffff;
                cells[cellCount].rxl = rxl;

                //LOG_DEBUG("gcy's cell:%d,%d,%d,%d,%x,%d", cellCount, _mcc, _mnc, lac, cellid, rxl);
            }

            if(0 != mcc)
            {
                isCellGet = EAT_TRUE;
                cellNo = cellCount + 1;//7
            }
            else
            {
                isCellGet = EAT_FALSE;
            }
        }
    }

    return;
}

static eat_bool gps_DuplicateCheck(LOCAL_GPS *pre_gps, LOCAL_GPS *gps)
{
    short cellid[6], last_cellid[6];
    short lac[6], last_lac[6];
    //short temp_cellid, temp_lac;
    int i=0, j=0, count=0;
    double distance = 0;

    if(pre_gps->isGpsFixed != gps->isGpsFixed)
    {
        LOG_DEBUG("gps_type is different.");
        return EAT_FALSE;
    }
    else
    {
        if(pre_gps->isGpsFixed == EAT_TRUE)
        {

            distance = getdistance(pre_gps,gps);
            if(distance <= 10 )//if the distance change 10m ,push the information of GPS
            {
                LOG_DEBUG("GPS is the same. %f, %f.", pre_gps->gps.latitude, gps->gps.latitude);
                return EAT_TRUE;
            }
            else
            {
                LOG_DEBUG("GPS is different. %f, %f.", pre_gps->gps.latitude, gps->gps.latitude);
                return EAT_FALSE;
            }

        }
        else
        {
            //CELL
            if((pre_gps->cellInfo.mcc != gps->cellInfo.mcc) || \
               (pre_gps->cellInfo.mnc != gps->cellInfo.mnc) || \
               (pre_gps->cellInfo.cellNo != gps->cellInfo.cellNo) || \
               (pre_gps->cellInfo.cell[0].lac != gps->cellInfo.cell[0].lac) || \
               (pre_gps->cellInfo.cell[0].cellid != gps->cellInfo.cell[0].cellid))
            {
                LOG_DEBUG("CELL is different for the first parameters.");
                return EAT_FALSE;
            }
            else
            {
                for(i=0; i<6; i++)
                {
                    last_cellid[i] = pre_gps->cellInfo.cell[i+1].cellid;
                    last_lac[i] = pre_gps->cellInfo.cell[i+1].lac;
                    cellid[i] = gps->cellInfo.cell[i+1].cellid;
                    lac[i] = gps->cellInfo.cell[i+1].lac;
                }

                count = 0;
                for(i=0; i<6; i++)
                {
                    for(j=0; j<6; j++)
                    {
                        if(last_cellid[i] == cellid[j] && last_lac[i] == lac[j])
                        {
                            count++;
                        }
                    }
                }

                if(count >= 3)
                {
                    LOG_DEBUG("CELL is the same, count = %d.", count);
                    return EAT_TRUE;
                }
                else
                {
                    LOG_DEBUG("CELL is different, count = %d.", count);
                    return EAT_FALSE;
                }
            }
        }
    }
}


static long double gps2rad(double d) //get rad value of latitude and longitude
{
    return d * pi / 180.f;
}

static long double getdistance(LOCAL_GPS *pre_gps, LOCAL_GPS *gps)  //get distance of new gps and the last gps
{
    long double radLat1 = gps2rad(pre_gps->gps.latitude);
    long double radLat2 = gps2rad(gps->gps.latitude);
    long double a = radLat1 - radLat2;
    long double b = gps2rad(pre_gps->gps.longitude) - gps2rad(gps->gps.longitude);

    long double s = 2 * asin(sqrt(sin(a/2)*sin(a/2)+cos(radLat1)*cos(radLat2)*sin(b/2)*sin(b/2)));
    LOG_DEBUG("Lat1:%lf,Lat2:%lf,Lon1:%lf,Lon2:%lf",pre_gps->gps.latitude,gps->gps.latitude,pre_gps->gps.longitude,gps->gps.longitude);

    s = s * EARTH_RADIUS;

    return s ;
}



