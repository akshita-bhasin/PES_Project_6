/*
 * @File  : logger.h
 * @Brief : contains function prototypes and includes for logger implementation
 * @Author : Akshita Bhasin and Madhukar Arora
 * @Created On : 11/1/2019
 */


#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "timestamp.h"
#include "fsl_debug_console.h"

typedef enum
{
	Test,
	Debug,
	Status
}log_level;

typedef enum
{
	Adc16_init,
	DACTask,
	DSPTask,
	ADCTask,
	Dma_CallBack,
	Circular_buf_init,
	Circular_buf_free,
	Circular_buf_reset,
	Circular_buf_put2,
	Circular_buf_get,
	Circular_buf_empty,
	Circular_buf_full,
	Circular_buf_capacity,
	Circular_buf_size,
	Circular_buf_initialized,
	Circular_buf_valid,
	Circular_buffer_realloc,
	LoggerTimerCallback,
	Timestamp_Init,
	Get_timestamp,
	Turn_on_LED_color,
	Toggle_LED_color
}function_name;

//function prototypes
char* get_func_name(function_name func_name);
char* get_log_level(log_level logLevel);
void log_string_detail(log_level logLevel, function_name func_name, char * str);

// Log_integer – display an integer
void log_integer(double num, char type);

// Log_char - display a character
void log_char(char ch);

void log_string(char * str);

#endif /* LOGGER_H_ */
