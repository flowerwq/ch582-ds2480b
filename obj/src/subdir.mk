################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Main.c \
../src/appinfo.c \
../src/bmp.c \
../src/configtool.c \
../src/display.c \
../src/gpio.c \
../src/liblightmodbus-impl.c \
../src/modbus.c \
../src/oled.c \
../src/sensor.c \
../src/storage.c \
../src/uart.c \
../src/uid.c \
../src/version.c \
../src/worktime.c 

OBJS += \
./src/Main.o \
./src/appinfo.o \
./src/bmp.o \
./src/configtool.o \
./src/display.o \
./src/gpio.o \
./src/liblightmodbus-impl.o \
./src/modbus.o \
./src/oled.o \
./src/sensor.o \
./src/storage.o \
./src/uart.o \
./src/uid.o \
./src/version.o \
./src/worktime.o 

C_DEPS += \
./src/Main.d \
./src/appinfo.d \
./src/bmp.d \
./src/configtool.d \
./src/display.d \
./src/gpio.d \
./src/liblightmodbus-impl.d \
./src/modbus.d \
./src/oled.d \
./src/sensor.d \
./src/storage.d \
./src/uart.d \
./src/uid.d \
./src/version.d \
./src/worktime.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -DDEBUG=1 -I"../StdPeriphDriver/inc" -I"../src/include" -I"../src/include/utils" -I"../src/liblightmodbus-3.0/include" -I"../RVMSIS" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

