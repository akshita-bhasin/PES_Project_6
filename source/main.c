/*
 * main.c
 *  Created on: Dec 05, 2019
 *  Author: Akshita Bhasin & Madhukar Arora
 *  Leverage Code : FreeRTOS, DMA, ADC and DAC SDK examples
 */

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include <arm_math.h>

#include "circularbuffer.h"
#include "led_control.h"
#include "timestamp.h"
#include "mode.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_dac.h"
#include "fsl_adc16.h"
#include "fsl_dma.h"
#include "fsl_dmamux.h"
#include "pin_mux.h"
#include "MKL25Z4.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define mainDAC_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define mainADC_TASK_PRIORITY (configMAX_PRIORITIES - 4)
#define mainDSP_TASK_PRIORITY (configMAX_PRIORITIES - 3)

/* The rate at which data is sent to the queue, specified in milliseconds, and
converted to ticks using the portTICK_PERIOD_MS constant. */
#define dacTASKPERIODMS (100 / portTICK_PERIOD_MS)
#define adcTASKPERIODMS (100 / portTICK_PERIOD_MS)

/* The period of the example software timer, specified in milliseconds, and
converted to ticks using the portTICK_PERIOD_MS constant. */
#define mainSOFTWARE_TIMER_PERIOD_MS (100 / portTICK_PERIOD_MS)

//#define mainQUEUE_LENGTH (1)

#define BUFF_LENGTH 64
#define DMA_CHANNEL 0
#define DMA_SOURCE 63

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if PGM_2
static void dacTask(void *pvParameters);
static void adcTask(void *pvParameters);
static void dspTask(void *pvParameters);
#endif

volatile uint32_t deciseconds = 0;
static volatile uint8_t dac_index = 0;
extern log_level log_level_a;

dma_handle_t g_DMA_Handle;
uint16_t dac_lookup_table[50];
cbuf_handle_t adcBuffer;
char dac_format[20];
uint8_t program_count=0;

TaskHandle_t ADC_Handle, DAC_Handle, DSP_Handle;

/***********************************************
function name : dac_lookup_init
parameters : void
return type : void
@brief : initializes value for the lookup table
}
*************************************************/
void dac_lookup_init(void)
{
	for(uint8_t index = 0; index < 50; index++)
	{
		dac_lookup_table[index] = (uint16_t)((sin(2*PI* (float) index/(float) 50) + 2) * (float) 4096 / 3.3);
	}
}

adc16_config_t adc16ConfigStruct;
adc16_channel_config_t adc16ChannelConfigStruct;
uint16_t adc_array[64];
uint8_t adc_index = 0;
uint32_t counter = 0;

SemaphoreHandle_t semMutex;


uint16_t dsp_buffer[64];
/* User callback function for DMA transfer. */
dma_transfer_config_t transferConfig;


/************************************************
function name : DMA_Callback
parameters : dma_handle_t *handle
return type : void
@brief : handles DMA Complete interrupt
}
*************************************************/
#if PGM_2
void DMA_Callback(dma_handle_t *handle, void *param)
{
	log_string_detail(log_level_a, Dma_CallBack, "DMA Transfer Complete");
	xTaskResumeFromISR(DSP_Handle);
	DMA0->DMA[0].DSR_BCR |= DMA_DSR_BCR_DONE(1);
}
#endif

TimerHandle_t xDACSoftwareTimer = NULL;

#if PGM_1
static void vDACTimerCallback(TimerHandle_t xTimer);
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
int main(void)
{

    /* Init board hardware. */
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    LED_RED_INIT(1);
    LED_BLUE_INIT(1);
    LED_GREEN_INIT(1);
    TimeStamp_Init();
//    SysTick_Config(48000000L/10L);

    /* DAC Init */

    dac_config_t dacConfigStruct;

    DAC_GetDefaultConfig(&dacConfigStruct);
    DAC_Init(DAC0, &dacConfigStruct);
    DAC_Enable(DAC0, true);             /* Enable output. */
    DAC_SetBufferReadPointer(DAC0, 0U);

    dac_lookup_init();


    /* ADC Init */
    ADC16_GetDefaultConfig(&adc16ConfigStruct);
    ADC16_Init(ADC0, &adc16ConfigStruct);
    ADC16_EnableHardwareTrigger(ADC0, false); /* Make sure the software trigger is used. */
    adc16ChannelConfigStruct.channelNumber = 0U;
    adc16ChannelConfigStruct.enableInterruptOnConversionCompleted = false;
    adc16ChannelConfigStruct.enableDifferentialConversion = false;

	if (kStatus_Success == ADC16_DoAutoCalibration(ADC0))
	{
		log_string_detail(Status, Adc16_init, "ADC16_DoAutoCalibration() Done.\r\n");
	}
	else
	{
		log_string_detail(Status, Adc16_init, "ADC16_DoAutoCalibration() Failed.\r\n");
	}

	//create the ADC buffer
	adcBuffer = circular_buf_init(64); //xQueueCreate(64,sizeof(uint16_t));
	//dspBuffer =  xQueueCreate(64,sizeof(uint16_t));
	if(adcBuffer != 0)
	{
		log_string_detail(Status, Adc16_init, "ADC Buffer Created Successfully\r\n");
	}
	else
	{
		log_string_detail(Status, Adc16_init,"Failed to create ADC Queue\r\n");
	}

    for(uint8_t i = 0; i < 64; i++)
    	dsp_buffer[i] = 0;

    /* DMA Init */

    /* Configure DMAMUX */
    DMAMUX_Init(DMAMUX0);
    DMAMUX_SetSource(DMAMUX0, DMA_CHANNEL, DMA_SOURCE);
    DMAMUX_EnableChannel(DMAMUX0, DMA_CHANNEL);
    /* Configure DMA one shot transfer */
    DMA_Init(DMA0);
    DMA_CreateHandle(&g_DMA_Handle, DMA0, DMA_CHANNEL);

    //Referred to sd example for free_rtos_mutex
    //semMutex = xSemaphoreCreateCounting(5,0);
    vSemaphoreCreateBinary(semMutex);

    xSemaphoreGive(semMutex);
    uint8_t sem_count = uxSemaphoreGetCount(semMutex);
    if(!sem_count)
    {
    	turn_off_led_color('G');
    	xSemaphoreTake(semMutex, 0);
    	counter++;
    }
    else
    {
    	turn_on_led_color('G');
    	xSemaphoreGive(semMutex);
    	counter++;
    }
    sem_count = uxSemaphoreGetCount(semMutex);
    if(!sem_count)
    {
    	turn_off_led_color('G');
    	xSemaphoreGive(semMutex);
    	counter++;
    }
    else
    {
    	turn_on_led_color('G');
    	counter++;
    }

#if PGM_2
    xTaskCreate(dacTask, "DAC Task", configMINIMAL_STACK_SIZE + 100, NULL, mainDAC_TASK_PRIORITY, &DAC_Handle);
    xTaskCreate(adcTask, "ADC Task", configMINIMAL_STACK_SIZE + 100, NULL, mainADC_TASK_PRIORITY, &ADC_Handle);
    xTaskCreate(dspTask, "DSP Task", configMINIMAL_STACK_SIZE + 400, NULL, mainDSP_TASK_PRIORITY, &DSP_Handle);
#endif
#if PGM_1
    xDACSoftwareTimer = xTimerCreate("DACTimer",mainSOFTWARE_TIMER_PERIOD_MS,pdTRUE,(void *)0,vDACTimerCallback);

        if(xTimerStart(xDACSoftwareTimer, 0) == pdTRUE)
        	log_string_detail(log_level_a, DACTask, "In DAC Logger");
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    for (;;)
        ;
}

uint8_t led_block=0;
/***********************************************
function name : adcTask
parameters : void *pvParameters
return type : void
@brief : for performing ADC operations
*************************************************/
#if PGM_2
static void adcTask(void *pvParameters)
{
//	char adc_val[20];
//	uint8_t i;
	uint8_t count=0;
	TickType_t PreviousWakeTime = xTaskGetTickCount();
    for (;;)
    {
    	ADC16_SetChannelConfig(ADC0, 0U, &adc16ChannelConfigStruct);
    	while (0U == (kADC16_ChannelConversionDoneFlag & ADC16_GetChannelStatusFlags(ADC0, 0U)));
        adc_array[adc_index] = ADC16_GetChannelConversionValue(ADC0, 0U);
        circular_buf_put2(adcBuffer, adc_array[adc_index]);
        adc_index++;
        if(circular_buf_full(adcBuffer) == buffer_full)
        {
        	program_count += 1;
//        	for(uint8_t i = 0; i < adc_index; i++)
//        	{
//        		//circular_buf_get(adcBuffer, &adc_array[index]);
////        		sprintf(adc_val, "ADC Values %d : %d\n\r", i, adc_array[i]);
////        		log_string_detail(log_level_a, ADCTask, adc_val);
//        	}

        	turn_on_led_color('B');
        	led_block = 5;
        	//}
			//vTaskDelay(500 / portTICK_PERIOD_MS);
        	log_string("DMA Start");
            DMA_SetCallback(&g_DMA_Handle, DMA_Callback, NULL);
            DMA_PrepareTransfer(&transferConfig, adc_array, sizeof(adc_array[0]), dsp_buffer, sizeof(dsp_buffer[0]), 128,
                                kDMA_MemoryToMemory);
            DMA_SubmitTransfer(&g_DMA_Handle, &transferConfig, kDMA_EnableInterrupt);
        	DMA_StartTransfer(&g_DMA_Handle);
        	adcBuffer->count=0;
        }

        vTaskDelayUntil(&PreviousWakeTime, adcTASKPERIODMS);
    }
}
#endif

/***********************************************
function name : dacTask
parameters : void *pvParameters
return type : void
@brief : for performing DAC operations
*************************************************/
#if PGM_2
static void dacTask(void *pvParameters)
{
	TickType_t PreviousWakeTime = xTaskGetTickCount();
    for (;;)
    {
    	DAC_SetBufferValue(DAC0, 0U, dac_lookup_table[dac_index]);
    	dac_index++;
    	if(dac_index == 50)
    		dac_index = 0;
    	if(led_block != 0)
    	{
    		led_block--;
    	}
    	else
    		toggle_led_color('G');

    	//toggle_led_color('G');
        vTaskDelayUntil(&PreviousWakeTime, dacTASKPERIODMS);
    }
}
#endif


/***********************************************
function name : dspTask
parameters : void *pvParameters
return type : void
@brief : for performing DSP operations
*************************************************/
#if PGM_2
static void dspTask(void *pvParameters)
{
	    for (;;)
	    {
			char maxi[30], mini[30];
			double average = 0;
			double variance_sum = 0;
			uint16_t max = 0;
			uint16_t min = 65535;
			double standard_deviation = 0;
	        for(uint8_t index = 0; index < BUFF_LENGTH; index++)
	        {
	        	average += dsp_buffer[index];
	        	if(dsp_buffer[index] > max)
	        	{
	        		max = dsp_buffer[index];
	        	}
	        	if(dsp_buffer[index] < min)
	        	{
	        		min = dsp_buffer[index];
	        	}
	        }

	        for(uint8_t index = 0; index < BUFF_LENGTH; index++)
	        {
	        	variance_sum = variance_sum + pow((dsp_buffer[index] - (average/64)), 2);
	        }
	        variance_sum = variance_sum / BUFF_LENGTH;
	        standard_deviation = sqrt(variance_sum);
	        sprintf(maxi, "Max     : %d\n\r", max);
	        sprintf(mini, "Min     : %d\n\r", min);
	        log_string_detail(log_level_a, DSPTask, "Average : ");
	        log_integer((average/64), 'F');
	        log_string_detail(log_level_a, DSPTask, maxi);
	        log_string_detail(log_level_a, DSPTask, mini);
	        log_string_detail(log_level_a, DSPTask, "Standard Deviation : ");
	        log_integer(standard_deviation, 'F');

	        if(program_count == 5)
	        {
	        	xSemaphoreGive(semMutex);
	        	LED_RED_OFF();
	        	LED_GREEN_OFF();
	        	LED_BLUE_OFF();
	        	vTaskDelete(ADC_Handle);
	        	vTaskDelete(DAC_Handle);
	        	vTaskDelete(DSP_Handle);
	        }
	        vTaskSuspend(DSP_Handle);
	    }
}
#endif


/***********************************************
fucntion name : vDACTimerCallback
parameters : TimerHandle_t
return type : void
@brief : DAC Timer Callback for Program 1
*************************************************/
#if PGM_1
static void vDACTimerCallback(TimerHandle_t xTimer)
{
    	DAC_SetBufferValue(DAC0, 0U, dac_lookup_table[dac_index]);
    	dac_index++;
    	if(dac_index == 50)
    		dac_index = 0;

    	toggle_led_color('B');
    	sprintf(dac_format, "DAC Values: \t%d", dac_lookup_table[dac_index]);

    	if(log_level_a == 1)
    		log_string_detail(Debug, DACTask, dac_format);
}
#endif

/***********************************************
function name : vApplicationTickHook
parameters : void
return type : void
@brief :  tick hood is executed every tick
*************************************************/
void vApplicationTickHook(void)
{
//    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static uint32_t ulCount = 0;

    ulCount++;
    if (ulCount >= 500UL)
    {
        ulCount = 0UL;
    }
}

/***********************************************
fucnction name : vApplicationMallocFailedHook
parameters : void
return type : void
@brief : malloc failed hook
*************************************************/
void vApplicationMallocFailedHook(void)
{
    for (;;)
        ;
}


/***********************************************
fucntion name : vApplicationStackOverflowHook
parameters : void
return type : void
@brief : stack overflow hook
*************************************************/
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
    (void)pcTaskName;
    (void)xTask;

    /* Run time stack overflow checking is performed if
    configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected.  pxCurrentTCB can be
    inspected in the debugger if the task name passed into this function is
    corrupt. */
    for (;;)
        ;
}

/***********************************************
fucntion name : vApplicationIdleHook
parameters : void
return type : void
@brief : idle hook
*************************************************/
void vApplicationIdleHook(void)
{
    volatile size_t xFreeStackSpace;

    /* The idle task hook is enabled by setting configUSE_IDLE_HOOK to 1 in
    FreeRTOSConfig.h.

    This function is called on each cycle of the idle task.  In this case it
    does nothing useful, other than report the amount of FreeRTOS heap that
    remains unallocated. */
    xFreeStackSpace = xPortGetFreeHeapSize();

    if (xFreeStackSpace > 100)
    {
        /* By now, the kernel has allocated everything it is going to, so
        if there is a lot of heap remaining unallocated then
        the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
        reduced accordingly. */
    }
}
