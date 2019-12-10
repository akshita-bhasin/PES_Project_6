/*
 * @File Name  : logger.c
 * @Brief : contains logger implementation
 * @Author : Akshita Bhasin and Madhukar Arora
 * @Created On : 11/1/2019
 */

#include "logger.h"

log_level log_level_a = 2;
char timestamp_format[100];

/* function name : log_string_detail
 * return type : void
 * parameters :logLevel - variable of enum type log_level, funcName - variable of enum type function_name, char *str - points to string
 * @brief : Uses the log level and the status of the function
 *          and prints the details
 */
void log_string_detail(log_level logLevel, function_name funcName, char * str)
{
	timestampt_t timestamp_value;
	timestamp_value = get_timestamp();
	char * log = get_log_level(logLevel);
	PRINTF("\r\n%s",log);
	char * func = get_func_name(funcName);
	PRINTF("%s",func);
 	PRINTF("\tTime from SysTick: \t%d: %d: %d: %d\t",  timestamp_value.hour, timestamp_value.minute, timestamp_value.second, timestamp_value.decisec);
	PRINTF("%s", str);
}

/* function name : log_string
 * return type : void
 * parameters : char *str - pointer to a string
 * @brief : prints a debug message to the terminal
 */
void log_string(char * str)
{
	timestampt_t timestamp_value;
	timestamp_value = get_timestamp();
 	PRINTF(timestamp_format,"\tTime from SysTick: \t%d: %d: %d: %d\n\r",  timestamp_value.hour, timestamp_value.minute, timestamp_value.second, timestamp_value.decisec);
	PRINTF("%s\r\n", str);


}

/* function name : log_char
 * return type : void
 * parameters : char ch
 * @brief : prints a single character to the terminal
 */
void log_char(char ch)
{
	timestampt_t timestamp_value;
	timestamp_value = get_timestamp();
 	PRINTF("Time from SysTick: \t%d: %d: %d: %d\n\r",  timestamp_value.hour, timestamp_value.minute, timestamp_value.second, timestamp_value.decisec);
	PRINTF("%c", ch);
}

/* function name : log_integer
 * return type : void
 * parameters : double num - number, char type - number is decimal or hexadecimal
 * @brief : prints a number to the terminal depending on its type
 */
void log_integer(double num, char type)
{
	char print_format[100];
	if(type == 'd')
	{
		PRINTF(print_format, "%d\n\r", num);
	}
	else if((type == 'h') | (type=='H'))
	{
		PRINTF(print_format, "0x%X\n\r", num);
	}
	else if((type == 'f') | (type=='F'))
	{
		PRINTF("%f\n\r", num);
	}
	else
	{
		PRINTF(print_format, "%d\n\r", num);
	}
}

/* function name : get_func_name
 * return type : char*
 * parameters : function_name func_name - func_name is an variable of enum type function_name
 * @brief : returns a string
 */
char* get_func_name(function_name func_name)
{
	switch(func_name)
	{
	if(log_level_a == 1)
	{
		case Adc16_init: return "ADC16_Init- initializes ADC16 configuration: ";
							break;
		case DACTask: return "DAC_Task- Task to perform DAC operations: ";
							break;
		case DSPTask: return "DSP_Task- Task to perform DSP calculations: ";
							break;
		case ADCTask: return "ADC_Task- Task to perform ADC operations: ";
							break;
		case Dma_CallBack: return "Dma_Callback- call back funcion to do DMA Transfer: ";
							break;
		case Circular_buf_init: return "Circular_buf_init- uses malloc and initializes circular buffer: ";
						break;
		case Circular_buf_free: return "Circular_buf_free- frees allocated buffer and circular buffer pointer: ";
							break;
		case Circular_buf_reset: return "Circular_buf_reset- resets the circular buffer to init state: ";
							break;
		case Circular_buf_put2: return "Circular_buf_put2- writes data into buffer: ";
							break;
		case Circular_buf_get: return "Circular_buf_get- reads data from buffer: ";
								break;
		case Circular_buf_empty: return "Circular_buf_empty- checks if circular is empty: ";
								break;
		case Circular_buf_full: return "Circular_buf_full- checks if circular is full: ";
								break;
		case Circular_buf_capacity: return "Circular_buf_capacity- returns capacity of circular buffer: ";
								break;
		case Circular_buf_size: return "Circular_buf_size- gets size of circular buffer: ";
								break;
		case Circular_buf_initialized: return "Circular_buf_initialized- checks if circular buffer is initialized or not: ";
								break;
		case Circular_buf_valid: return "Circular_buf_valid- check if circular buffer is valid or not: ";
								break;
		case Circular_buffer_realloc: return "Circular_buffer_realloc - reallocates memeory when an overflow happens: ";
										break;
		case LoggerTimerCallback: return "vLoggerTimerCallback- callback function for logger timer deciseconds calculation: ";
								break;
		case Timestamp_Init: return "Timestamp_Init- init function to create logger software timer: ";
								break;
		case Get_timestamp: return "Get_timestamp- gets timestamp using Software Timer with a 0.1 second and displays in HH:mm:ss:n!: ";
							break;
		case Turn_on_LED_color: return "turn_on_LED_Color- turns on chosen color of LED: ";
								break;
		case Toggle_LED_color: return "toggle_LED_Color- toggles chosen color of LED: ";
								break;
	}
	else
		return " ";
	}
	return " ";
}

/* function name : get_log_level
 * return type : char*
 * parameters : log_level logLevel- logLevel is a variable of enum type log_level
 * @brief : returns a string
 */
char* get_log_level(log_level logLevel)
{
	switch(logLevel)
	{
		case Test: return "Test: ";
					break;
		case Status: return "Status: ";
					break;
		case Debug: return "Debug: ";
					break;
	}
	return " ";
}



