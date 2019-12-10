/*
 * timestamp.h
 *
 *  Created on: Nov 14, 2019
 *      Author: akshh
 */

#ifndef TIMESTAMP_H_
#define TIMESTAMP_H_

#include <stdint.h>
#include "MKL25Z4.h"
#include "FreeRTOS.h"
#include "timers.h"

typedef struct {
    uint8_t hour;   /*!< Range from 0 to 23.*/
    uint8_t minute; /*!< Range from 0 to 59.*/
    uint8_t second;
    uint8_t decisec;
}timestampt_t;

//void Init_SysTick(void);
void TimeStamp_Init(void);
timestampt_t get_timestamp(void);
void vLoggerTimerCallback(TimerHandle_t xTimer);

#endif /* TIMESTAMP_H_ */
