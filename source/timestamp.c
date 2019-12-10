/*
 * File : timestamp.c
 * Created on: Nov 14, 2019
 * Author: Akshita Bhasin & Madhukar Arora
 * Brief : Contains code for timestamp implementation
 */

#include "timestamp.h"

extern uint32_t deciseconds;
#define mainSOFTWARE_TIMER_PERIOD_MS (100 / portTICK_PERIOD_MS)
TimerHandle_t xLoggerExampleTimer = NULL;
void TimeStamp_Init(void) {
    xLoggerExampleTimer = xTimerCreate("LoggerTimer", mainSOFTWARE_TIMER_PERIOD_MS, pdTRUE, (void *)0, vLoggerTimerCallback);
    xTimerStart(xLoggerExampleTimer, 0);
}

/*
 * function name : get_timestamp
 * parameters : void
 * return type : timestamp_t - returns a structure
 * brief : function to get time elapsed since the start of the application
 */
timestampt_t get_timestamp(void)
{
	timestampt_t timestamp_value;
	uint16_t seconds, minutes, hours;
	seconds = deciseconds / 10;
	minutes = seconds / 60;
	hours = minutes / 60;

	timestamp_value.hour = hours;
	timestamp_value.minute = (minutes - (hours*60));
	timestamp_value.second = (seconds - (minutes*60));
	timestamp_value.decisec = (deciseconds - (seconds*10));

	return timestamp_value;
}

void vLoggerTimerCallback(TimerHandle_t xTimer)
{
    /* The timer has expired.  Count the number of times this happens.  The
    timer that calls this function is an auto re-load timer, so it will
    execute periodically. */
    deciseconds++;
}
