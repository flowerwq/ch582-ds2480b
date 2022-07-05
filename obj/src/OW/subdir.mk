################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/OW/MDC04_app.c \
../src/OW/MDC04_driver.c \
../src/OW/MY_ow.c 

OBJS += \
./src/OW/MDC04_app.o \
./src/OW/MDC04_driver.o \
./src/OW/MY_ow.o 

C_DEPS += \
./src/OW/MDC04_app.d \
./src/OW/MDC04_driver.d \
./src/OW/MY_ow.d 


# Each subdirectory must supply rules for building sources it contributes
src/OW/%.o: ../src/OW/%.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -DDEBUG=1 -I"../StdPeriphDriver/inc" -I"D:\workspace\mounriver_studio\CH582M-app\src\include" -I"D:\workspace\mounriver_studio\CH582M-app\src\liblightmodbus-3.0\include" -I"../RVMSIS" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

