################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ds2480b/commun.c \
../src/ds2480b/onewire.c \
../src/ds2480b/uart-ds2480.c 

OBJS += \
./src/ds2480b/commun.o \
./src/ds2480b/onewire.o \
./src/ds2480b/uart-ds2480.o 

C_DEPS += \
./src/ds2480b/commun.d \
./src/ds2480b/onewire.d \
./src/ds2480b/uart-ds2480.d 


# Each subdirectory must supply rules for building sources it contributes
src/ds2480b/%.o: ../src/ds2480b/%.c
	@	@	riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -DDEBUG=1 -I"../StdPeriphDriver/inc" -I"../src/include" -I"../src/include/utils" -I"../src/liblightmodbus-3.0/include" -I"../RVMSIS" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@	@

