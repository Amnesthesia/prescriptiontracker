#include "prescription.h"


prescription::prescription()
{
    date = time(NULL);
}


prescription::prescription(person* dr, person* ptnt, time_t dt)
{
    this->SetPatient(ptnt);
    this->SetDoctor(dr);
    this->SetTime(dt);
}

prescription::prescription(person* dr, person* ptnt, medicine* drg, time_t dt)
{
    this->SetPatient(ptnt);
    this->SetDoctor(dr);
    this->SetDrug(drg);
    this->SetTime(dt);
}


// Setters

/**
 ** Sets the hour specified by integer parameter h
 **
 ** @param integer h        Hour to set time to
 */
void prescription::SetHour(int h)
{
    tmpDate->tm_hour = h;
}


/**
 ** Sets the minute specified by integer parameter m
 **
 ** @param integer m        Minute to set time to
 */
void prescription::SetMinute(int m)
{
    tmpDate->tm_min = m;
}

/**
 ** Sets the second specified by integer parameter s
 **
 ** @param integer s        Second to set time to
 */
void prescription::SetSecond(int s)
{
    tmpDate->tm_sec = s;
}

/**
 ** Sets the day specified by integer parameter d
 **
 ** @param integer d        Day of the month to set time to
 */
void prescription::SetDay(int d)
{
    tmpDate->tm_mday = d;
}


/**
 ** Sets the month specified by integer parameter m
 **
 ** @param integer m        Month to set time to
 */
void prescription::SetMonth(int m)
{
    tmpDate->tm_mon = m;
}

/**
 ** Sets the year specified by integer parameter y
 **
 ** @param integer y        Year to set time to
 */
void prescription::SetYear(int y)
{
    tmpDate->tm_year = y;
}


/**
 ** Sets the timestamp manually
 **
 ** @param integer d        Timestamp
 */
void prescription::SetDate(time_t d)
{
    date = d;
}

/**
 ** Sets the timestamp manually (AND updates tmpDate accordingly)
 **
 ** @param integer d        Timestamp
 */
void prescription::SetTime(time_t d)
{
    tmpDate = localtime(&d);
    SetDate(d);
}






