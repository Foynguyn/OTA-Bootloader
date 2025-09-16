################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/Src/stm32f103xx_bootloader.c \
../drivers/Src/stm32f103xx_core_driver.c \
../drivers/Src/stm32f103xx_flash_driver.c \
../drivers/Src/stm32f103xx_gpio_drivers.c \
../drivers/Src/stm32f103xx_usart_driver.c 

OBJS += \
./drivers/Src/stm32f103xx_bootloader.o \
./drivers/Src/stm32f103xx_core_driver.o \
./drivers/Src/stm32f103xx_flash_driver.o \
./drivers/Src/stm32f103xx_gpio_drivers.o \
./drivers/Src/stm32f103xx_usart_driver.o 

C_DEPS += \
./drivers/Src/stm32f103xx_bootloader.d \
./drivers/Src/stm32f103xx_core_driver.d \
./drivers/Src/stm32f103xx_flash_driver.d \
./drivers/Src/stm32f103xx_gpio_drivers.d \
./drivers/Src/stm32f103xx_usart_driver.d 


# Each subdirectory must supply rules for building sources it contributes
drivers/Src/%.o drivers/Src/%.su drivers/Src/%.cyclo: ../drivers/Src/%.c drivers/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F1 -DSTM32F103C8Tx -c -I../Inc -I"D:/bootloader_project/OTA-Bootloader/bootloader/drivers/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-drivers-2f-Src

clean-drivers-2f-Src:
	-$(RM) ./drivers/Src/stm32f103xx_bootloader.cyclo ./drivers/Src/stm32f103xx_bootloader.d ./drivers/Src/stm32f103xx_bootloader.o ./drivers/Src/stm32f103xx_bootloader.su ./drivers/Src/stm32f103xx_core_driver.cyclo ./drivers/Src/stm32f103xx_core_driver.d ./drivers/Src/stm32f103xx_core_driver.o ./drivers/Src/stm32f103xx_core_driver.su ./drivers/Src/stm32f103xx_flash_driver.cyclo ./drivers/Src/stm32f103xx_flash_driver.d ./drivers/Src/stm32f103xx_flash_driver.o ./drivers/Src/stm32f103xx_flash_driver.su ./drivers/Src/stm32f103xx_gpio_drivers.cyclo ./drivers/Src/stm32f103xx_gpio_drivers.d ./drivers/Src/stm32f103xx_gpio_drivers.o ./drivers/Src/stm32f103xx_gpio_drivers.su ./drivers/Src/stm32f103xx_usart_driver.cyclo ./drivers/Src/stm32f103xx_usart_driver.d ./drivers/Src/stm32f103xx_usart_driver.o ./drivers/Src/stm32f103xx_usart_driver.su

.PHONY: clean-drivers-2f-Src

