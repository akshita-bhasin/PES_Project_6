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

/* TODO Add any manufacture supplied header files necessary for CMSIS functions
to be available here. */
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

/* Priorities at which the tasks are created.  The event semaphore task is
given the maximum priority of ( configMAX_PRIORITIES - 1 ) to ensure it runs as
soon as the semaphore is given. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define mainQUEUE_SEND_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define mainEVENT_SEMAPHORE_TASK_PRIORITY (configMAX_PRIORITIES - 1)

/* The rate at which data is sent to the queue, specified in milliseconds, and
converted to ticks using the portTICK_PERIOD_MS constant. */
#define dacTASKPERIODMS (100 / portTICK_PERIOD_MS)

/* The period of the example software timer, specified in milliseconds, and
converted to ticks using the portTICK_PERIOD_MS constant. */
#define mainSOFTWARE_TIMER_PERIOD_MS (100 / portTICK_PERIOD_MS)

/* The number of items the queue can hold.  This is 1 as the receive task
will remove items as they are added, meaning the send task should always find
the queue empty. */
#define mainQUEUE_LENGTH (1)

#define BUFF_LENGTH 64
#define DMA_CHANNEL 0
#define DMA_SOURCE 63
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*
 * The queue send and receive tasks as described in the comments at the top of
 * this file.
 */
static void dacTask(void *pvParameters);
static void adcTask(void *pvParameters);
static void dspTask(void *pvParameters);
//static void prvQueueSendTask(void *pvParameters);

/*
 * The callback function assigned to the example software timer as described at
 * the top of this file.
 */
static void vExampleTimerCallback(TimerHandle_t xTimer);

volatile uint32_t deciseconds = 0;
static volatile uint8_t dac_index = 0;
//static volatile uint32_t ulCountOfItemsReceivedOnQueue = 0;
//static volatile uint32_t ulCountOfReceivedSemaphores = 0;
extern log_level log_level_a;

dma_handle_t g_DMA_Handle;
uint16_t dac_lookup_table[50];
cbuf_handle_t adcBuffer;
char dac_format[5];
void dac_lookup_init(void)
{
	for(uint8_t index = 0; index < 50; index++)
	{
		dac_lookup_table[index] = (uint16_t)((sin(2*PI* (float) index/(float) 50) + 2) * (float) 4096 / 3.3);
	}
}

adc16_config_t adc16ConfigStruct;
adc16_channel_config_t adc16ChannelConfigStruct;
uint16_t adc_buffer[64];
uint8_t adc_index = 0;

TaskHandle_t fuckyou;

uint16_t dsp_buffer[64];
/* User callback function for DMA transfer. */
dma_transfer_config_t transferConfig;
void DMA_Callback(dma_handle_t *handle, void *param)
{
	xTaskResumeFromISR(fuckyou);
	DMA0->DMA[0].DSR_BCR |= DMA_DSR_BCR_DONE(1);
}



/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
int main(void)
{

    TimerHandle_t xExampleSoftwareTimer = NULL;

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
		log_string_detail(Status, Adc16_init, "ADC Queue Created Successfully\r\n");
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

    /* Create the queue used by the queue send and queue receive tasks. */
   // xQueue = xQueueCreate(/* The number of items the queue can hold. */
                   //       mainQUEUE_LENGTH,
                          /* The size of each item the queue holds. */
                          //sizeof(uint32_t));

    /* Create the semaphore used by the FreeRTOS tick hook function and the
    event semaphore task. */
//    vSemaphoreCreateBinary(xEventSemaphore);

    /* Create the queue receive task as described in the comments at the top
    of this    file. */
    xTaskCreate(dacTask, "DAC Task", configMINIMAL_STACK_SIZE + 300, NULL, mainQUEUE_RECEIVE_TASK_PRIORITY, NULL);

    /* Create the queue send task in exactly the same way.  Again, this is
    described in the comments at the top of the file. */
//    xTaskCreate(adcTask, "ADC Task", configMINIMAL_STACK_SIZE + 300, NULL, mainQUEUE_SEND_TASK_PRIORITY, NULL);
//    xTaskCreate(dspTask, "DSP Task", configMINIMAL_STACK_SIZE + 300, NULL, configMAX_PRIORITIES - 1, &fuckyou);

    /* Create the task that is synchronised with an interrupt using the
    xEventSemaphore semaphore. */
//    xTaskCreate(prvEventSemaphoreTask, "Sem", configMINIMAL_STACK_SIZE + 166, NULL, mainEVENT_SEMAPHORE_TASK_PRIORITY,
//                NULL);

    /* Create the software timer as described in the comments at the top of
    this file. */
//    xExampleSoftwareTimer = xTimerCreate("LEDTimer", mainSOFTWARE_TIMER_PERIOD_MS, pdTRUE, (void *)0, vExampleTimerCallback);
//
//    /* Start the created timer.  A block time of zero is used as the timer
//    command queue cannot possibly be full here (this is the first timer to
//    be created, and it is not yet running). */
//    xTimerStart(xExampleSoftwareTimer, 0);

    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details.  */
    for (;;)
        ;
}

/*!
 * @brief Timer callback.
 */
//static void vExampleTimerCallback(TimerHandle_t xTimer)
//{
//    /* The timer has expired.  Count the number of times this happens.  The
//    timer that calls this function is an auto re-load timer, so it will
//    execute periodically. */
//    deciseconds++;
//}

/*!
 * @brief Task prvQueueSendTask periodically sending message.
 */
static void adcTask(void *pvParameters)
{
	TickType_t PreviousWakeTime = xTaskGetTickCount();
    for (;;)
    {
        /* Wait until something arrives in the queue - this task will block
        indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
        FreeRTOSConfig.h. */
    	ADC16_SetChannelConfig(ADC0, 0U, &adc16ChannelConfigStruct);
    	while (0U == (kADC16_ChannelConversionDoneFlag & ADC16_GetChannelStatusFlags(ADC0, 0U)));
        adc_buffer[adc_index] = ADC16_GetChannelConversionValue(ADC0, 0U);
        circular_buf_put2(adcBuffer, adc_buffer[adc_index]);
        adc_index++;
        //if(adc_index == 64)
        if(circular_buf_full(adcBuffer))
        {
        	//adc_index = 0;
        	adcBuffer->count=0;
        	for(uint8_t index = 0; index < 64; index++)
        	{
        		PRINTF("ADC Values %d : %d\n\r", index, adc_buffer[index]);
        	}
//        	PRINTF("DMA Start\n\r");
            DMA_SetCallback(&g_DMA_Handle, DMA_Callback, NULL);
            DMA_PrepareTransfer(&transferConfig, adc_buffer, sizeof(adc_buffer[0]), dsp_buffer, sizeof(dsp_buffer[0]), sizeof(adc_buffer),
                                kDMA_MemoryToMemory);
            DMA_SubmitTransfer(&g_DMA_Handle, &transferConfig, kDMA_EnableInterrupt);
        	DMA_StartTransfer(&g_DMA_Handle);
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
        /* Wait until something arrives in the queue - this task will block
        indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
        FreeRTOSConfig.h. */
//    	PRINTF("DAC Values : %d\n\r", dac_lookup_table[dac_index]);
    	DAC_SetBufferValue(DAC0, 0U, dac_lookup_table[dac_index]);
    	dac_index++;
    	if(dac_index == 50)
    		dac_index = 0;

    	toggle_led_color('B');
    	sprintf(dac_format, "DAC Values: \t%d", dac_lookup_table[dac_index]);


    	if(log_level_a == 1)
    		log_string_detail(Debug, DACTask, dac_format);
        vTaskDelayUntil(&PreviousWakeTime, dacTASKPERIODMS);
    }
}

static void dspTask(void *pvParameters)
{
		uint32_t average = 0;
		float variance_sum = 0;
		uint16_t max = 0;
		uint16_t min = 0xffff;
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
	        variance_sum = variance_sum / (float) BUFF_LENGTH;
	        standard_deviation = sqrt(variance_sum);

	        PRINTF("Average : %d\n\r", (uint16_t)(float) average / 64);
	        PRINTF("Max     : %d\n\r", max);
	        PRINTF("Min     : %d\n\r", min);
	        PRINTF("SD      : %d\n\r", standard_deviation);
	        vTaskSuspend(fuckyou);
	    }
}

/*!
 * @brief task prvEventSemaphoreTask is waiting for semaphore.
 */
//static void prvEventSemaphoreTask(void *pvParameters)
//{
//    for (;;)
//    {
//        /* Block until the semaphore is 'given'. */
//        xSemaphoreTake(xEventSemaphore, portMAX_DELAY);
//
//        /* Count the number of times the semaphore is received. */
//        ulCountOfReceivedSemaphores++;
//
//        PRINTF("Event task is running.\r\n");
//    }
//}

/*!
 * @brief tick hook is executed every tick.
 */
void vApplicationTickHook(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static uint32_t ulCount = 0;

    /* The RTOS tick hook function is enabled by setting configUSE_TICK_HOOK to
    1 in FreeRTOSConfig.h.

    "Give" the semaphore on every 500th tick interrupt. */
    ulCount++;
    if (ulCount >= 500UL)
    {
        /* This function is called from an interrupt context (the RTOS tick
        interrupt),    so only ISR safe API functions can be used (those that end
        in "FromISR()".

        xHigherPriorityTaskWoken was initialised to pdFALSE, and will be set to
        pdTRUE by xSemaphoreGiveFromISR() if giving the semaphore unblocked a
        task that has equal or higher priority than the interrupted task. */
//        xSemaphoreGiveFromISR(xEventSemaphore, &xHigherPriorityTaskWoken);
        ulCount = 0UL;
    }

    /* If xHigherPriorityTaskWoken is pdTRUE then a context switch should
    normally be performed before leaving the interrupt (because during the
    execution of the interrupt a task of equal or higher priority than the
    running task was unblocked).  The syntax required to context switch from
    an interrupt is port dependent, so check the documentation of the port you
    are using.

    In this case, the function is running in the context of the tick interrupt,
    which will automatically check for the higher priority task to run anyway,
    so no further action is required. */
}

/*!
 * @brief Malloc failed hook.
 */
void vApplicationMallocFailedHook(void)
{
    /* The malloc failed hook is enabled by setting
    configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.

    Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
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
