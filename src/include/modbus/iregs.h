#ifndef __MODBUS_IREGS_H__
#define __MODBUS_IREGS_H__
#include "stdint.h"
#include "lightmodbus/lightmodbus.h"

typedef enum mb_ireg_addr{
	MB_IREG_ADDR_BASE = 0,
	MB_IREG_ADDR_TEMP = MB_IREG_ADDR_BASE,
	MB_IREG_ADDR_RH,
	MB_IREG_ADDR_MAX,
} mb_ireg_addr_t;

void modbus_ireg_update(mb_ireg_addr_t addr, uint16_t value);
void modbus_iregs_init();
ModbusError modbus_ireg_callback(void *ctx, 
	const ModbusRegisterCallbackArgs *args,
	ModbusRegisterCallbackResult *out);

#endif
