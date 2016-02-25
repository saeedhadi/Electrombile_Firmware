
#include <string.h>

#include <eat_fs_type.h>
#include <eat_fs.h>
#include <eat_fs_errcode.h>
#include <eat_modem.h>
#include <eat_interface.h>
#include <eat_uart.h>

#include <eat_periphery.h>

#include "setting.h"
#include "debug.h"
#include "log.h"
#include "msg.h"
#include "client.h"
#include "socket.h"
#include "timer.h"
#include "rtc.h"
#include "fs.h"
#include "mileage.h"

#define EAT_ADC0 EAT_PIN23_ADC1
#define EAT_ADC1 EAT_PIN24_ADC2
#define ADC1_PERIOD 10  //sampling period : 10ms
#define ADC_RELATIVE_VALUE 3*1000/103.f   //3K&100K resistance ,change unit to mV

#define MILEAGEFILE_NAME   L"C:\\mileage"


DumpVoltage mileage_storage = {0};
static unsigned int adcvalue;               //usually used to detect the value of the ADC1
static unsigned int adcvalue_start;         //Before starting the detection of electricity


extern double mileage;


static void msg_mileage_send(MSG_MILEAGE_REQ msg_mileage);
static eat_bool mileage_reload(void);

/*
*fun:load the default mileage value or that in mileagefile
*/
eat_bool mileage_restore(void)
{

    LOG_DEBUG("mileage initial...");
    mileage_initial();
    eat_sleep(100);
    LOG_DEBUG("restore mileage from file");
    return mileage_reload();
}
/*
*reload the value in mileagefile
*/
static eat_bool mileage_reload(void)
{

    DumpVoltage storage;
    FS_HANDLE fh;
    eat_bool ret = EAT_FALSE;
    int rc , i;

    /*mileage reload*/
    fh = eat_fs_Open(MILEAGEFILE_NAME, FS_READ_ONLY);
    if(EAT_FS_FILE_NOT_FOUND == fh)
    {
        LOG_INFO("mileage file not exists.");
        return EAT_TRUE;
    }

    if (fh < EAT_FS_NO_ERROR)
    {
        LOG_ERROR("read mileage file fail, rc: %d", fh);
        return EAT_FALSE;
    }

    rc = eat_fs_Read(fh, &storage, sizeof(DumpVoltage), NULL);
    if (EAT_FS_NO_ERROR == rc)
    {
        LOG_DEBUG("read mileage file success.");
        /*load and record the value in mileagefile*/
        for(i = 0;i < MAX_MILEAGE_LEN;i++)
        {
            mileage_storage.dump_mileage[i] = storage.dump_mileage[i];
            mileage_storage.voltage[i] = storage.dump_mileage[i];
        }

        ret = EAT_TRUE;
    }
    else
    {
        LOG_ERROR("read file fail, and return error: %d", fh);
    }

    eat_fs_Close(fh);

    return ret;
}


int cmd_deletemileage(const unsigned char* cmdString, unsigned short length)
{
    eat_fs_error_enum fs_Op_ret;

    fs_Op_ret = (eat_fs_error_enum)eat_fs_Delete(MILEAGEFILE_NAME);
    if(EAT_FS_NO_ERROR != fs_Op_ret && EAT_FS_FILE_NOT_FOUND != fs_Op_ret)
    {
        LOG_ERROR("Delete mileagefile Fail,and Return Error is %d",fs_Op_ret);
        return EAT_FALSE;
    }
    else
    {
        LOG_DEBUG("Delete mileagefile Success");
    }
    return EAT_TRUE;
}


void mileage_initial(void)
{
    LOG_INFO("milegae initial to default value.");

    regist_cmd("deletemileage", cmd_deletemileage);

    /*go to adc_mileageinit_proc to judge the type of the battery*/
    eat_adc_get(EAT_ADC1, ADC1_PERIOD, adc_mileageinit_proc);
}
/*
*fun:save the value in mileage_storage
*/
eat_bool mileage_save(void)
{
    FS_HANDLE fh;
    int rc,i;
    UINT writedLen;
    eat_bool ret = EAT_FALSE;

    DumpVoltage storage;

    for(i = 0;i < MAX_MILEAGE_LEN; i++)
    {
        storage.dump_mileage[i] = mileage_storage.dump_mileage[i];
        storage.voltage[i] = mileage_storage.voltage[i];
    }


    LOG_INFO("save mileage...");

    fh = eat_fs_Open(MILEAGEFILE_NAME, FS_READ_WRITE | FS_CREATE);
    if(EAT_FS_NO_ERROR <= fh)
    {
        LOG_DEBUG("open file success, fh=%d.", fh);

        rc = eat_fs_Write(fh, &storage, sizeof(DumpVoltage), &writedLen);
        if(EAT_FS_NO_ERROR == rc && sizeof(DumpVoltage) == writedLen)
        {
            LOG_DEBUG("write file success.");
        }
        else
        {
            LOG_ERROR("write file failed, and Return Error is %d, writedLen is %d.", rc, writedLen);
        }
    }
    else
    {
        LOG_ERROR("open file failed, fh=%d.", fh);
    }
    eat_fs_Close(fh);

    return ret;
}
/*
*fun:read 40 times ADC1's value ,average and judge the battery type
*/
void adc_mileageinit_proc(EatAdc_st* adc)
{
    u16 adcvalue = 0;
    int i;
    static u16 value[40],count = 0;

    if(adc->pin == EAT_ADC1)
    {
        value[count] = adc->v;
        count++;
        if(count < 40)
        {
            if(count == 39)
            {
                /*end this detect next*/
                eat_adc_get(EAT_ADC1, NULL, adc_mileageinit_proc);
            }
            return;
        }
    }
    else if(adc->pin == EAT_ADC0)
    {
        return;
    }
    /* average the value of the 40 times' */
    for(i = 0;i < 40;i++)
    {
        adcvalue += value[i];
    }
    adcvalue /= 40;

    count = 0;//reset count

    if(adcvalue > 52*ADC_RELATIVE_VALUE)       //adcvalue>52V,assert 60V
    {
        LOG_INFO("the valtage is %d,assert 60V/TYPE BATTERY", adcvalue);
        for(i = 0;i <MAX_MILEAGE_LEN;i++)
        {
            mileage_storage.dump_mileage[i] = 0;
            /*电压按照设计标准上调3V，计算范围为12V*/
            mileage_storage.voltage[i] = (63-12*i/MAX_MILEAGE_LEN)*ADC_RELATIVE_VALUE;
        }
    }
    else if(adcvalue > 40*ADC_RELATIVE_VALUE)   //adcvalue>40V,assert 48V
    {
        LOG_INFO("the valtage is %d,assert 48V/TYPE BATTERY", adcvalue);
        for(i = 0;i <MAX_MILEAGE_LEN;i++)
        {
            mileage_storage.dump_mileage[i] = 0;
            /*电压按照设计标准上调3V，计算范围为12V*/
            mileage_storage.voltage[i] = (51-12*i/MAX_MILEAGE_LEN)*ADC_RELATIVE_VALUE;
        }
    }
    else if(adcvalue <= 40*ADC_RELATIVE_VALUE)  //adcvalue<40,assert 36V
    {
        LOG_INFO("the valtage is %d,assert 36V/TYPE BATTERY", adcvalue);
        for(i = 0;i <MAX_MILEAGE_LEN;i++)
        {
            mileage_storage.dump_mileage[i] = 0;
            /*电压按照设计标准上调3V，计算范围为12V*/
            mileage_storage.voltage[i] = (39-12*i/MAX_MILEAGE_LEN)*ADC_RELATIVE_VALUE;
        }
    }
    mileage_save();
}
/*
*fun:when the trip is over detect the battery value and caculate the mileage
*/
void adc_mileageend_proc(EatAdc_st* adc)
{
    static short count = 0;
    static int voltage[40] = {0};
    int average_voltage = 0;
    short start = 0,end = 0;
    float average_mileage = 0.0;

    if(count < 40)
    {
        if(adc->pin == EAT_ADC1)
        {
            voltage[count] = adc->v;
            count++;
            if(count == 39)
            {
                /*end this detect next*/
                eat_adc_get(EAT_ADC1,NULL,adc_mileageend_proc);
            }
            return;
        }
        else if(adc->pin == EAT_ADC0)
        {
            return;
        }
    }
    else
    {
        /* average the value of the 40 times' */
        for(count = 0;count <40;count++)
        {
            average_voltage += voltage[count];
        }
        average_voltage /= 40;

        while(adcvalue_start < mileage_storage.voltage[start++]);
        while(average_voltage > mileage_storage.voltage[end++]);
        --start;
        --end;
        if(start > end)//if the value of start is ahead the value of end,abort it
        {
            LOG_INFO("data is invalid , abort...");
            /*abort this mileage data*/
        }
        else if(start == end)
        {
            //if the trip is only in one block,judge if it is larger than the old
            if(mileage > mileage_storage.dump_mileage[start])
            {
                mileage_storage.dump_mileage[start] = mileage;
                /*there to save the mileage surface*/
                mileage_save();

            }
            else
            {
                LOG_INFO("data is invalid , abort...");
                /*abort this mileage data*/
            }
        }
        else
        {
            /*average the mileage and write it in the mileage surface*/

            average_mileage = mileage / (start - end);
            for(count = start;count < end;count ++)
            {
                mileage_storage.dump_mileage[count] += average_mileage;
                mileage_storage.dump_mileage[count] /= 2.0;
            }
            count = 0;
            mileage_save();
        }

        count = 0;
        return;

    }
}

/*
*fun:when the trip start, detect the battery value to start to caculate the mileage
*/
void adc_mileagestart_proc(EatAdc_st* adc)
{
    static short count = 0;
    static int voltage[40] = {0};
    int average_voltage = 0;

    if(count < 40)
    {
        if(adc->pin == EAT_ADC1)
        {
            voltage[count] = adc->v;
            count++;
            if(count == 39)
            {
                /*end this detect next*/
                eat_adc_get(EAT_ADC1,NULL,adc_mileagestart_proc);
            }
        }
        else if(adc->pin == EAT_ADC0)
        {
            return;
        }
    }
    else
    {
        /* average the value of the 40 times' */
        for(count = 0;count <40;count++)
        {
            average_voltage += voltage[count];
        }
        average_voltage /= 40;

        /*record the start value*/
        adcvalue_start = average_voltage;

        count = 0;
        return;

    }
}
/*
*fun:when the electric vehicle stop,detect the battery value 3min once(not in use)
*/
void adc_voltage_proc(EatAdc_st* adc)
{
    static short count = 0;
    static int voltage[40] = {0};
    int average_voltage = 0;

    if(count < 40)
    {
        if(adc->pin == EAT_ADC1)
        {
            voltage[count] = adc->v;
            count++;
            if(count == 39)
            {
                /*end this detect next*/
                eat_adc_get(EAT_ADC1,NULL,adc_voltage_proc);
            }
        }
        else if(adc->pin == EAT_ADC0)
        {
            return;
        }
    }
    else
    {
        /* average the value of the 40 times' */
        for(count = 0;count <40;count++)
        {
            average_voltage += voltage[count];
        }
        average_voltage /= 40;

        /* record the adcvalue */
        adcvalue = average_voltage;

        count = 0;
        return;

    }
}




void mileagehandle(short MILEAGE_STATE)
{
    static MSG_MILEAGE_REQ msg_mileage;
    static eat_bool mileage_flag = EAT_FALSE;
    time_t timestamp;


    timestamp = rtc_getTimestamp();
    LOG_DEBUG("timestamp:%ld",timestamp);

    if(MILEAGE_START == MILEAGE_STATE && mileage_flag == EAT_FALSE)
    {
        msg_mileage.starttime = timestamp;
        msg_mileage.mileage = 0;
        mileage_flag = EAT_TRUE;
        mileage = 0.f;
    }
    else if(MILEAGE_STOP == MILEAGE_STATE && mileage_flag == EAT_TRUE)
    {
        msg_mileage.endtime = timestamp;
        msg_mileage.mileage = (int)mileage;
        msg_mileage_send(msg_mileage);
        LOG_DEBUG("send this mileage :%d m",msg_mileage.mileage);
        mileage_flag = EAT_FALSE;
    }
    else if(MILEAGE_RESTORE == MILEAGE_STATE && mileage_flag == EAT_FALSE)
    {
        eat_adc_get(EAT_ADC1,ADC1_PERIOD,adc_mileageend_proc);
        return;
    }
    else
    {
        mileage_flag = EAT_FALSE;
        return;
    }
}

static void msg_mileage_send(MSG_MILEAGE_REQ msg_mileage)
{
    u8 msgLen = sizeof(MSG_MILEAGE_REQ);
    MSG_MILEAGE_REQ* msg = alloc_msg(CMD_ITINERARY, msgLen);
    msg->endtime = msg_mileage.endtime;
    msg->starttime = msg_mileage.starttime;
    msg->mileage = (int)msg_mileage.mileage;
    LOG_INFO("send the mileage");
    socket_sendData(msg, msgLen);
}
/*
*fun: get the dump battery
*para: void
*return:(char)dump_battery - x%
*/

char get_battery(void)
{
    int i = 0;
    char ret;
    float miles = 0 , dump_miles = 0;

    while(mileage_storage.voltage[i++] > adcvalue);
    if(i < 0 || i > MAX_MILEAGE_LEN)
    {
        LOG_INFO("get_mileage ERROR ! Voltage too low: %d",adcvalue);
        return 0;
    }
    for(i-- ;i < MAX_MILEAGE_LEN;i++)
    {
        dump_miles += mileage_storage.dump_mileage[i];
    }
    for(i = 0;i < MAX_MILEAGE_LEN;i++)
    {
        miles += mileage_storage.dump_mileage[i];
    }
    ret = (char)((dump_miles / miles)*100);
    return ret;

}

/*
*fun: get the dump mileage
*para: void
*return:(char)dump_mileage - km
*/
char get_mileage(void)
{
    int i =0;
    float miles = 0;
    char ret;
    mileage_reload();
    while(mileage_storage.voltage[i++] > adcvalue);
    if(i < 0 || i > MAX_MILEAGE_LEN)
    {
        LOG_INFO("get_mileage ERROR ! Voltage too low: %d",adcvalue);
        return 0;
    }
    for(i-- ;i < MAX_MILEAGE_LEN;i++)
    {
        miles += mileage_storage.dump_mileage[i];
    }
    miles /= 1000.f;
    if((miles - (int)miles) > 0.5)
    {
        ret = (char)(miles+1);
    }
    else
    {
        ret = (char)(miles);
    }
    return ret;


}


/*
*fun: when  electric motor car is stopping , detect voltage 3min once,
*when electric motor car is starting , stop to detect voltage
*para: short
*value: DETECTVOLTAGE_START
*       DETECTVOLTAGE_STOP
*/
void detectvoltage_timer(short operation)
{
    if(DETECTVOLTAGE_START == operation)
    {
        LOG_INFO("TIMER_VOLTAGE_GET start!");
//        eat_timer_start(TIMER_VOLTAGE_GET, setting.detectvolatge_timer_peroid);
    }
    else if(DETECTVOLTAGE_STOP == operation)
    {
        LOG_INFO("TIMER_VOLTAGE_GET stop!");
        eat_timer_stop(TIMER_VOLTAGE_GET);
    }
}





