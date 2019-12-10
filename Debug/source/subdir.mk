################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/circularbuffer.c \
../source/led_control.c \
../source/logger.c \
../source/main.c \
../source/mtb.c \
../source/retarget_itm.c \
../source/semihost_hardfault.c \
../source/timestamp.c 

OBJS += \
./source/circularbuffer.o \
./source/led_control.o \
./source/logger.o \
./source/main.o \
./source/mtb.o \
./source/retarget_itm.o \
./source/semihost_hardfault.o \
./source/timestamp.o 

C_DEPS += \
./source/circularbuffer.d \
./source/led_control.d \
./source/logger.d \
./source/main.d \
./source/mtb.d \
./source/retarget_itm.d \
./source/semihost_hardfault.d \
./source/timestamp.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MKL25Z128VLK4 -DCPU_MKL25Z128VLK4_cm0plus -DDEBUG -DFSL_RTOS_FREE_RTOS -DFRDM_KL25Z -DFREEDOM -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=1 -DSDK_DEBUGCONSOLE_UART -DSDK_DEBUGCONSOLE_ITM -D__MCUXPRESSO -D__USE_CMSIS -DARM_MATH_CM0PLUS -DSDK_DEBUGCONSOLE=1 -I../board -I../source -I../ -I../drivers -I../CMSIS -I../utilities -I../freertos -I../startup -O0 -fno-common -g -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


