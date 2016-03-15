/*
 * data.c
 *
 *  Created on: 2015��7��9��
 *      Author: jk
 */


#include "data.h"

typedef struct queue
{
    int front;
    int rear;
    GPS gps[MAX_GPS_COUNT];
}QUEUE;


QUEUE gps_queue = {0, 0};


/*
 * to judge whether the queue is full
 */
eat_bool gps_isQueueFull(void)
{
    return gps_queue.front == (gps_queue.rear + 1) % MAX_GPS_COUNT;
}

/*
 * to judge whether the queue is empty
 */
eat_bool gps_isQueueEmpty(void)
{
    return gps_queue.front == gps_queue.rear;
}

/*
 * put a GPS in the queue
 */
eat_bool gps_enqueue(GPS* gps)
{
    if (gps_isQueueFull())
    {
        return EAT_FALSE;
    }

    /*memcpy is not useful, dont know why*/
    //memcpy(gps_queue.gps[gps_queue.rear], gps, sizeof(GPS));

    gps_queue.gps[gps_queue.rear].latitude = gps->latitude;
    gps_queue.gps[gps_queue.rear].longitude = gps->longitude;
    gps_queue.gps[gps_queue.rear].speed = gps->speed;
    gps_queue.gps[gps_queue.rear].course = gps->course;

    gps_queue.rear = (gps_queue.rear + 1) % MAX_GPS_COUNT;

    return EAT_TRUE;
}


/*
 * get a GPS in the queue
 */
eat_bool gps_dequeue(GPS* gps)
{
    if (!gps)
    {
        return EAT_FALSE;
    }

    if (gps_isQueueEmpty())
    {
        return EAT_FALSE;
    }

    /*memcpy is not useful, dont know why*/
    //memcpy(gps, gps_queue.gps[gps_queue.front], sizeof(GPS));

    gps->latitude = gps_queue.gps[gps_queue.front].latitude;
    gps->longitude = gps_queue.gps[gps_queue.front].longitude;
    gps->speed = gps_queue.gps[gps_queue.front].speed;
    gps->course = gps_queue.gps[gps_queue.front].course;

    gps_queue.front = (gps_queue.front + 1) % MAX_GPS_COUNT;

    return EAT_TRUE;
}

/*
 * get the queue's actual size
 */
int gps_size(void)
{
    return (gps_queue.rear - gps_queue.front + MAX_GPS_COUNT) % MAX_GPS_COUNT;
}
