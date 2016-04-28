#ifndef USER_INC_ITINERARY_H_
#define USER_INC_ITINERARY_H_


int itinerary_store(int starttime, int miles, int endtime);

int itinerary_get(int* starttime, int* miles, int* endtime);

int itinerary_delete(void);


#define ITINERARYFILE_NAME L"C:\\itinerary"

typedef struct ITINERARY
{
    int starttime;
    int miles;
    int endtime;
}ITINERARY;



#endif /* USER_INC_ITINERARY_H_ */

