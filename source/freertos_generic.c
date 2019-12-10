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

/* The period of the example software timer, specified in milliseconds, and
converted to ticks using the portTICK_PERIOD_MS constant. */
#define mainSOFTWARE_TIMER_PERIOD_MS (100 / portTICK_PERIOD_MS)

#define mainQUEUE_LENGTH (1)

#define BUFF_LENGTH 64
#define DMA_CHANNEL 0
#define DMA_SOURCE 63

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void dacTask(void *pvParameters);
static void adcTask(void *pvParameters);
static void dspTask(void *pvParameters);

volatile uint32_t deciseconds = 0;
static volatile uint8_t dac_index = 0;
extern log_level log_level_a;

dma_handle_t g_DMA_Handle;
uint16_t dac_lookup_table[50];
cbuf_handle_t adcBuffer;
char dac_format[20];
uint8_t program_count=0;

TaskHandle_t ADC_Handle, DAC_Handle, DSP_Handle;

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

SemaphoreHandle_t semMutex;


uint16_t dsp_buffer[64];
/* User callback function for DMA transfer. */
dma_transfer_config_t transferConfig;
void DMA_Callback(dma_handle_t *handle, void *param)
{
	log_string_detail(log_level_a, Dma_CallBack, "DMA Transfer Complete");
	xTaskResumeFromISR(DSP_Handle);
	DMA0->DMA[0].DSR_BCR |= DMA_DSR_BCR_DONE(1);
}

TimerHandle_t xDACSoftwareTimer = NULL;

static void vDACTimerCallback(TimerHandle_t xTimer);

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

    /* DMA Init */

    /* Configure DMAMUX */
    DMAMUX_Init(DMAMUX0);
    DMAMUX_SetSource(DMAMUX0, DMA_CHANNEL, DMA_SOURCE);
    DMAMUX_EnableChannel(DMAMUX0, DMA_CHANNEL);
    /* Configure DMA one shot transfer */
    DMA_Init(DMA0);
    DMA_CreateHandle(&g_DMA_Handle, DMA0, DMA_CHANNEL);

    //Referred to sd example for free_rtos_mutex
    semMutex = xSemaphoreCreateMutex();


    xTaskCreate(dacTask, "DAC Task", configMINIMAL_STACK_SIZE + 100, NULL, mainDAC_TASK_PRIORITY, &DAC_Handle);
    xTaskCreate(adcTask, "ADC Task", configMINIMAL_STACK_SIZE + 100, NULL, mainADC_TASK_PRIORITY, &ADC_Handle);
    xTaskCreate(dspTask, "DSP Task", configMINIMAL_STACK_SIZE + 400, NULL, mainDSP_TASK_PRIORITY, &DSP_Handle);

//    xDACSoftwareTimer = xTimerCreate("DACTimer",mainSOFTWARE_TIMER_PERIOD_MS,pdTRUE,(void *)0,vDACTimerCallback);
//
//        if(xTimerStart(xDACSoftwareTimer, 0) == pdTRUE)
//        	log_string_detail(log_level_a, DACTask, "In DAC Logger");

    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    for (;;)
        ;
}

/*!
 * @brief Task prvQueueSendTask periodically sending message.
 */
static void adcTask(void *pvParameters)
{
	char adc_val[20];
	uint8_t i;
	TickType_t PreviousWakeTime = xTaskGetTickCount();
    for (;;)
    {
    	ADC16_SetChannelConfig(ADC0, 0U, &adc16ChannelConfigStruct);
    	while (0U == (kADC16_ChannelConversionDoneFlag & ADC16_GetChannelStatusFlags(ADC0, 0U)));
        adc_array[adc_index] = ADC16_GetChannelConversionValue(ADC0, 0U);
        circular_buf_put2(adcBuffer, adc_array[adc_index]);
        adc_index++;
    	//circular_buf_put2(&adcBuffer, ADC16_GetChannelConversionValue(ADC0, 0U));
        //if(adc_index == 64)
        if(circular_buf_full(adcBuffer) == buffer_full)
        {
        	program_count += 1;
        	//xSemaphoreTake(semMutex,(TickType_t) 100);
        	turn_on_led_color('R');
//        	for(uint8_t i = 0; i < adc_index; i++)
//        	{
//        		//circular_buf_get(adcBuffer, &adc_array[index]);
////        		sprintf(adc_val, "ADC Values %d : %d\n\r", i, adc_array[i]);
////        		log_string_detail(log_level_a, ADCTask, adc_val);
//        	}
        	log_string("DMA Start");
        	turn_on_led_color('B');
            DMA_SetCallback(&g_DMA_Handle, DMA_Callback, NULL);
            DMA_PrepareTransfer(&transferConfig, adc_array, sizeof(adc_array[0]), dsp_buffer, sizeof(dsp_buffer[0]), 128,
                                kDMA_MemoryToMemory);
            DMA_SubmitTransfer(&g_DMA_Handle, &transferConfig, kDMA_EnableInterrupt);
        	DMA_StartTransfer(&g_DMA_Handle);
        	adcBuffer->count=0;
        }

        vTaskDelayUntil(&PreviousWakeTime, dacTASKPERIODMS);
    }
}

/*!
 * @brief Task prvQueueReceiveTask waiting for message.
 */
static void dacTask(void *pvParameters)
{
	TickType_t PreviousWakeTime = xTaskGetTickCount();
    for (;;)
    {
//		if(semMutex != NULL)
//		{
//			if(xSemaphoreTake(semMutex,(TickType_t) 100) == pdTRUE)
//			{
//				turn_on_led_color('G');
//				xSemaphoreGive(semMutex);
//			}
//		}
    	toggle_led_color('G');
    	DAC_SetBufferValue(DAC0, 0U, dac_lookup_table[dac_index]);
    	dac_index++;
    	if(dac_index == 50)
    		dac_index = 0;

    	//toggle_led_color('B');
//    	sprintf(dac_format, "DAC Values: \t%d", dac_lookup_table[dac_index]);
//
//
//    	if(log_level_a == 1)
//    		log_string_detail(Debug, DACTask, dac_format);
        vTaskDelayUntil(&PreviousWakeTime, dacTASKPERIODMS);
    }
}

static void dspTask(void *pvParameters)
{
		char avg[30], maxi[30], mini[30], std_dev[30];
		uint32_t average = 0;
		float variance_sum = 0;
		uint16_t max = 0;
		uint16_t min = 65535;
		uint32_t standard_deviation = 0;
	    for (;;)
	    {
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
	        	variance_sum = variance_sum + pow((dsp_buffer[index] - average), 2);
	        }
	        variance_sum = variance_sum / BUFF_LENGTH;
	        standard_deviation = sqrt(variance_sum);
	        sprintf(avg, "Average : %d\n\r", average / 64);
	        sprintf(maxi, "Max     : %d\n\r", max);
	        sprintf(mini, "Min     : %d\n\r", min);
	        sprintf(std_dev, "SD      : %d\n\r", standard_deviation);
	        log_string_detail(log_level_a, DSPTask, avg);
	        log_string_detail(log_level_a, DSPTask, maxi);
	        log_string_detail(log_level_a, DSPTask, mini);
	        log_string_detail(log_level_a, DSPTask, std_dev);

	        if(program_count == 5)
	        {
	        	xSemaphoreGive(semMutex);
	        	vTaskDelete(ADC_Handle);
	        	vTaskDelete(DAC_Handle);
	        	vTaskDelete(DSP_Handle);
	        }
	        vTaskSuspend(DSP_Handle);
	    }
}

//static void vDACTimerCallback(TimerHandle_t xTimer)
//{
//	//TickType_t PreviousWakeTime = xTaskGetTickCount();
////		if(semMutex != NULL)
////		{
////			if(xSemaphoreTake(semMutex,(TickType_t) 100) == pdTRUE)
////			{
////				turn_on_led_color('G');
////				xSemaphoreGive(semMutex);
////			}
////		}
//    	DAC_SetBufferValue(DAC0, 0U, dac_lookup_table[dac_index]);
//    	dac_index++;
//    	if(dac_index == 50)
//    		dac_index = 0;
//
//    	toggle_led_color('B');
//    	sprintf(dac_format, "DAC Values: \t%d", dac_lookup_table[dac_index]);
//
//    	if(log_level_a == 1)
//    		log_string_detail(Debug, DACTask, dac_format);
//}

/*!
 * @brief tick hook is executed every tick.
 */
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

/*!
 * @brief Malloc failed hook.
 */
void vApplicationMallocFailedHook(void)
{
    for (;;)
        ;
}

/*!
 * @brief Stack overflow hook.
 */
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

/*!
 * @brief Idle hook.
 */
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
