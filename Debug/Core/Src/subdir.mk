################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/adc.c \
../Core/Src/adc_monitor.c \
../Core/Src/cli.c \
../Core/Src/clock.c \
../Core/Src/dma.c \
../Core/Src/fault_logger.c \
../Core/Src/fault_manager.c \
../Core/Src/heartbeat_monitor.c \
../Core/Src/led.c \
../Core/Src/main.c \
../Core/Src/memory_monitor.c \
../Core/Src/peripheral_monitor.c \
../Core/Src/ram_test.c \
../Core/Src/reset_manager.c \
../Core/Src/stm32l4xx_hal_msp.c \
../Core/Src/stm32l4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32l4xx.c \
../Core/Src/system_time.c \
../Core/Src/usart.c \
../Core/Src/watchdog_manager.c 

OBJS += \
./Core/Src/adc.o \
./Core/Src/adc_monitor.o \
./Core/Src/cli.o \
./Core/Src/clock.o \
./Core/Src/dma.o \
./Core/Src/fault_logger.o \
./Core/Src/fault_manager.o \
./Core/Src/heartbeat_monitor.o \
./Core/Src/led.o \
./Core/Src/main.o \
./Core/Src/memory_monitor.o \
./Core/Src/peripheral_monitor.o \
./Core/Src/ram_test.o \
./Core/Src/reset_manager.o \
./Core/Src/stm32l4xx_hal_msp.o \
./Core/Src/stm32l4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32l4xx.o \
./Core/Src/system_time.o \
./Core/Src/usart.o \
./Core/Src/watchdog_manager.o 

C_DEPS += \
./Core/Src/adc.d \
./Core/Src/adc_monitor.d \
./Core/Src/cli.d \
./Core/Src/clock.d \
./Core/Src/dma.d \
./Core/Src/fault_logger.d \
./Core/Src/fault_manager.d \
./Core/Src/heartbeat_monitor.d \
./Core/Src/led.d \
./Core/Src/main.d \
./Core/Src/memory_monitor.d \
./Core/Src/peripheral_monitor.d \
./Core/Src/ram_test.d \
./Core/Src/reset_manager.d \
./Core/Src/stm32l4xx_hal_msp.d \
./Core/Src/stm32l4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32l4xx.d \
./Core/Src/system_time.d \
./Core/Src/usart.d \
./Core/Src/watchdog_manager.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L452xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/adc.cyclo ./Core/Src/adc.d ./Core/Src/adc.o ./Core/Src/adc.su ./Core/Src/adc_monitor.cyclo ./Core/Src/adc_monitor.d ./Core/Src/adc_monitor.o ./Core/Src/adc_monitor.su ./Core/Src/cli.cyclo ./Core/Src/cli.d ./Core/Src/cli.o ./Core/Src/cli.su ./Core/Src/clock.cyclo ./Core/Src/clock.d ./Core/Src/clock.o ./Core/Src/clock.su ./Core/Src/dma.cyclo ./Core/Src/dma.d ./Core/Src/dma.o ./Core/Src/dma.su ./Core/Src/fault_logger.cyclo ./Core/Src/fault_logger.d ./Core/Src/fault_logger.o ./Core/Src/fault_logger.su ./Core/Src/fault_manager.cyclo ./Core/Src/fault_manager.d ./Core/Src/fault_manager.o ./Core/Src/fault_manager.su ./Core/Src/heartbeat_monitor.cyclo ./Core/Src/heartbeat_monitor.d ./Core/Src/heartbeat_monitor.o ./Core/Src/heartbeat_monitor.su ./Core/Src/led.cyclo ./Core/Src/led.d ./Core/Src/led.o ./Core/Src/led.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/memory_monitor.cyclo ./Core/Src/memory_monitor.d ./Core/Src/memory_monitor.o ./Core/Src/memory_monitor.su ./Core/Src/peripheral_monitor.cyclo ./Core/Src/peripheral_monitor.d ./Core/Src/peripheral_monitor.o ./Core/Src/peripheral_monitor.su ./Core/Src/ram_test.cyclo ./Core/Src/ram_test.d ./Core/Src/ram_test.o ./Core/Src/ram_test.su ./Core/Src/reset_manager.cyclo ./Core/Src/reset_manager.d ./Core/Src/reset_manager.o ./Core/Src/reset_manager.su ./Core/Src/stm32l4xx_hal_msp.cyclo ./Core/Src/stm32l4xx_hal_msp.d ./Core/Src/stm32l4xx_hal_msp.o ./Core/Src/stm32l4xx_hal_msp.su ./Core/Src/stm32l4xx_it.cyclo ./Core/Src/stm32l4xx_it.d ./Core/Src/stm32l4xx_it.o ./Core/Src/stm32l4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32l4xx.cyclo ./Core/Src/system_stm32l4xx.d ./Core/Src/system_stm32l4xx.o ./Core/Src/system_stm32l4xx.su ./Core/Src/system_time.cyclo ./Core/Src/system_time.d ./Core/Src/system_time.o ./Core/Src/system_time.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su ./Core/Src/watchdog_manager.cyclo ./Core/Src/watchdog_manager.d ./Core/Src/watchdog_manager.o ./Core/Src/watchdog_manager.su

.PHONY: clean-Core-2f-Src

