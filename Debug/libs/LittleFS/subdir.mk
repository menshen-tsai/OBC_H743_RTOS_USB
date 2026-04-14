################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libs/LittleFS/lfs.c \
../libs/LittleFS/lfs_util.c \
../libs/LittleFS/littlefs_qspi.c 

OBJS += \
./libs/LittleFS/lfs.o \
./libs/LittleFS/lfs_util.o \
./libs/LittleFS/littlefs_qspi.o 

C_DEPS += \
./libs/LittleFS/lfs.d \
./libs/LittleFS/lfs_util.d \
./libs/LittleFS/littlefs_qspi.d 


# Each subdirectory must supply rules for building sources it contributes
libs/LittleFS/%.o libs/LittleFS/%.su libs/LittleFS/%.cyclo: ../libs/LittleFS/%.c libs/LittleFS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I"D:/Prog/STM32_Projects/H7_Series/OBC_H743_Blink_Timer3_UART3_DMA_CAN_RTOS_MENU_LittleFS/libs/LittleFS" -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-libs-2f-LittleFS

clean-libs-2f-LittleFS:
	-$(RM) ./libs/LittleFS/lfs.cyclo ./libs/LittleFS/lfs.d ./libs/LittleFS/lfs.o ./libs/LittleFS/lfs.su ./libs/LittleFS/lfs_util.cyclo ./libs/LittleFS/lfs_util.d ./libs/LittleFS/lfs_util.o ./libs/LittleFS/lfs_util.su ./libs/LittleFS/littlefs_qspi.cyclo ./libs/LittleFS/littlefs_qspi.d ./libs/LittleFS/littlefs_qspi.o ./libs/LittleFS/littlefs_qspi.su

.PHONY: clean-libs-2f-LittleFS

