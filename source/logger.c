/*
 * @File Name  : logger.c
 * @Brief : contains logger implementation
 * @Author : Akshita Bhasin and Madhukar Arora
 * @Created On : 11/1/2019
 */

#include "logger.h"

log_level log_level_a = 1;
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
	PRINTF("%s\r\n", str);
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
 * parameters : uint8_t num - number, char type - number is decimal or hexadecimal
 * @brief : prints a number to the terminal depending on its type
 */
void log_integer(uint8_t num, char type)
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
	else
	{
		PRINTF(print_format, "%d\n\r", num);
	}

	timestampt_t timestamp_value;
	timestamp_value = get_timestamp();
 	PRINTF("Time from SysTick: \t%d: %d: %d: %d\n\r",  timestamp_value.hour, timestamp_value.minute, timestamp_value.second, timestamp_value.decisec);
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
#ifdef DEBUG_LOG
		case init_UART0: return "Init_UART0- initializes UART0: ";
							break;
		case Uart0_getchar: return "UART0_getchar- gets a character from the user when receiver is ready to receive: ";
							break;
		case Uart0_putchar: return "UART0_putchar- puts character back on terminal when transmitter is ready to transmit: ";
							break;
		case Tx_available: return "Tx_available- checks if transmitter available: ";
							break;
		case Rx_available: return "Rx_available- checks if receiver available: ";
							break;
		case Uart0_rx_chars_available: return "Uart0_rx_chars_available- size of receiver circular buffer available: ";
							break;
		case Uart0_get_rx_char: return "Uart0_get_rx_char- pops one item from the receiver buffer: ";
							break;
		case send_String_Poll: return "send_String_Poll- sends entire string of characters using UART polling: ";
							break;
		case send_String: return "send_String- sends string of characters using UART interrupt: ";
							break;
		case Uart_echo: return "Uart_echo- echo mode operations performed: ";
							break;
		case Uart_application: return "Uart_application- application mode operations performed: ";
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
		case Count_characters: return "Count_characters- keeps a count of alphabets received on UART: ";
								break;
		case Application_report: return "Application_report- used to print report in application mode: ";
								break;
		case Get_timestamp: return "Get_timestamp- gets timestamp using SysTick Timer with a 0.1 second and displays in HH:mm:ss:n!: ";
							break;
		case Turn_on_LED_color: return "turn_on_LED_Color- turns on chosen color of LED: ";
								break;
#endif

#ifdef NORMAL
		case init_UART0: return "Init_UART0: ";
							break;
		case Uart0_getchar: return "UART0_getchar: ";
							break;
		case Uart0_putchar: return "UART0_putchar: ";
							break;
		case Tx_available: return "Tx_available: ";
							break;
		case Rx_available: return "Rx_available: ";
							break;
		case Uart0_rx_chars_available: return "Uart0_rx_chars_available: ";
							break;
		case Uart0_get_rx_char: return "Uart0_get_rx_char: ";
							break;
		case send_String_Poll: return "send_String_Poll: ";
							break;
		case send_String: return "send_String: ";
							break;
		case Uart_echo: return "Uart_echo: ";
							break;
		case Uart_application: return "Uart_application: ";
							break;
		case Circular_buf_init: return "Circular_buf_init: ";
						break;
		case Circular_buf_free: return "Circular_buf_free: ";
							break;
		case Circular_buf_reset: return "Circular_buf_reset: ";
							break;
		case Circular_buf_put2: return "Circular_buf_put2: ";
							break;
		case Circular_buf_get: return "Circular_buf_get: ";
								break;
		case Circular_buf_empty: return "Circular_buf_empty: ";
								break;
		case Circular_buf_full: return "Circular_buf_full: ";
								break;
		case Circular_buf_capacity: return "Circular_buf_capacity: ";
								break;
		case Circular_buf_size: return "Circular_buf_size: ";
								break;
		case Circular_buf_initialized: return "Circular_buf_initialized: ";
								break;
		case Circular_buf_valid: return "Circular_buf_valid: ";
								break;
		case Count_characters: return "Count_characters: ";
								break;
		case Application_report: return "Application_report: ";
								break;
		case Get_timestamp: return "Get_timestamp: ";
							break;
		case Turn_on_LED_color: return "turn_on_LED_Color: ";
								break;
#endif
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



