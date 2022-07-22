#include "stdio.h"
#include "CH58x_common.h"
#include "worktime.h"
#include "modbus.h"
#include "configtool.h"
#include "appinfo.h"
#include "utils.h"
#include "uid.h"
#include "ds2480b/uart-ds2480.h"
#include "uart.h"

#define TAG "SENSOR"

#define MB_OPT_RESET 0
#define MB_OPT_UPGRADE 	1

#define MB_OPT_RELOAD	3

typedef  struct sensor_context {
	uint8_t flag_init:1;
	uint8_t flag_reset:1;
	uint64_t worktime;
}sensor_ctx_t;
static sensor_ctx_t sensor_ctx;

static bool baudrate_check(uint32_t baudrate){
	switch(baudrate){
		case 4800:
		case 9600:
		case 19200:
		case 38400:
		case 57600:
		case 115200:
		case 230400:
		case 380400:
		case 460800:
		case 921600:
			break;
		default:
			goto fail;
	}
	return true;
fail:
	return false;
}

static int sensor_reload_uart_config(){
	cfg_uart_t cfg_uart = {0};
	cfg_get_mb_uart(&cfg_uart);
	cfg_uart.baudrate = modbus_reg_get(MB_REG_ADDR_BAUDRATE_H);
	cfg_uart.baudrate <<= 16;
	cfg_uart.baudrate += modbus_reg_get(MB_REG_ADDR_BAUDRATE_L);
	if (!baudrate_check(cfg_uart.baudrate)){
		LOG_ERROR(TAG, "invalid baudrate(%lu)", cfg_uart.baudrate);
		goto fail;
	}
	LOG_DEBUG(TAG, "update uart config(baudrate:%lu)", cfg_uart.baudrate);
	cfg_update_mb_uart(&cfg_uart);
	return 0;
fail:
	return -1;
}

static int sensor_reload_mb_addr(){
	uint16_t tmp = 0;
	tmp = modbus_reg_get(MB_REG_ADDR_MB_ADDR);
	if (tmp < 1 || tmp > 247){
		LOG_ERROR(TAG, "invalid modbus address(%d)", tmp);
		modbus_reg_update(MB_REG_ADDR_MB_ADDR, tmp);
		goto fail;
	}
	LOG_DEBUG(TAG, "update mb_addr:%d", tmp);
	cfg_update_mb_addr((uint8_t)tmp);
	return 0;
fail:
	return -1;
}

static int sensor_reload_sn(){
	uint8_t sn[CFG_SN_LEN + 1] = {0};
	uint16_t *buf = modbus_reg_buf_addr(MB_REG_ADDR_UID_BUF_START);
	int bytes_remain = CFG_SN_LEN;
	int i = 0;
	if (!buf[0]){
		goto fail;
	}
	for(i = 0; i <= MB_REG_ADDR_UID_BUF_END - MB_REG_ADDR_UID_BUF_START; i ++){
		if (!buf[i]){
			break;
		}
		sn[i * 2] = buf[i] >> 8;
		if (sn[i * 2] > 127){
			LOG_ERROR(TAG, "invalid byte(0x%02X)", sn[i * 2]);
			goto fail;
		}
		bytes_remain --;
		if (!bytes_remain){
			break;
		}
		sn[i * 2 + 1] = buf[i] & 0xffU;
		if (sn[i * 2 + 1] > 127){
			LOG_ERROR(TAG, "invalid byte(0x%02X)", sn[i * 2 + 1]);
			goto fail;
		}
		bytes_remain --;
		if (!bytes_remain){
			break;
		}
	}
	sn[CFG_SN_LEN] = 0;
	if (sn[0]){
		LOG_DEBUG(TAG, "update sn: %s", sn);
		cfg_update_sn((char *)sn);
	}
	return 0;
fail:
	return -1;
}

/*
 * @brief Save configuration 
 */
static int sensor_reload_config(){
	sensor_reload_mb_addr();
	sensor_reload_uart_config();
	sensor_reload_sn();
	modbus_reload();
	return 0;
}

static int mb_addr_check(uint16_t value){
	return value >= 1 && value <= 247;
}

static int mb_reg_before_write(mb_reg_addr_t addr, uint16_t value){
	switch(addr){
		case MB_REG_ADDR_SAFE_ACCESS_CTRL:
			if (value && value != 0x3736 ){
				goto fail;
			}
			break;
		case MB_REG_ADDR_MB_ADDR:
			if (!mb_addr_check(value)){
				goto fail;
			}
			break;
		default:
			break;
	}
	return 0;
fail:
	return -1;
}

static int mb_opt_handle(uint16_t action){
	switch(action){
		case MB_OPT_RESET:
		case MB_OPT_UPGRADE:
			sensor_ctx.flag_reset = 1;
			break;
		case MB_OPT_RELOAD:
			sensor_reload_config();
			break;
		default:
			LOG_ERROR(TAG, "unknown operation(%d)", action);
			break;
	}
	return 0;
}

static int mb_reg_after_write(mb_reg_addr_t addr, uint16_t value){
	switch(addr){
		case MB_REG_ADDR_TEST_1:
			if (value != 0x5AA5){
				modbus_reg_update(MB_REG_ADDR_TEST_1, 0x5AA5);
			}
			break;
		case MB_REG_ADDR_TEST_2:
			modbus_reg_update(MB_REG_ADDR_TEST_2, ~value);
			break;
		case MB_REG_ADDR_OPT_CTRL:
			mb_opt_handle(value);
			break;
		case MB_REG_ADDR_SAFE_ACCESS_CTRL:
			if (0x3736 == value){
				modbus_sa_ctrl(1);
			}else if (0 == value){
				modbus_sa_ctrl(0);
			}
			break;
		default:
			break;
	}
	return 0;
}

int mb_init(){
	mb_callback_t mb_cbs  = {0};
	uint8_t mb_addr = 0;
	cfg_uart_t cfg_uart;
	uint8_t sn[CFG_SN_LEN] = {0};
	const appinfo_t *appinfo = appinfo_get();
	mb_cbs.before_reg_write = mb_reg_before_write;
	mb_cbs.after_reg_write = mb_reg_after_write;
	modbus_init(&mb_cbs);
	modbus_reg_update(MB_REG_ADDR_APP_STATE, MB_REG_APP_STATE_APP);
	modbus_reg_update(MB_REG_ADDR_APP_VID, appinfo->vid);
	modbus_reg_update(MB_REG_ADDR_APP_PID, appinfo->pid);
	modbus_reg_update(MB_REG_ADDR_VERSION_H, appinfo->version >> 16);
	modbus_reg_update(MB_REG_ADDR_VERSION_L, appinfo->version);
	modbus_reg_update(MB_REG_ADDR_HW_VERSION, appinfo->hw_version);
	modbus_reg_update(MB_REG_ADDR_TEST_1, 0x5AA5);
	cfg_get_mb_addr(&mb_addr);
	cfg_get_mb_uart(&cfg_uart);
	cfg_get_sn((char *)sn);
	modbus_reg_update_uid(sn, CFG_SN_LEN);
	modbus_reg_update(MB_REG_ADDR_MB_ADDR, mb_addr);
	modbus_reg_update(MB_REG_ADDR_BAUDRATE_H, cfg_uart.baudrate >> 16);
	modbus_reg_update(MB_REG_ADDR_BAUDRATE_L, cfg_uart.baudrate & 0xffffU);
	return 0;
}

static int uart_ds2480b(){
    uart_config_t cfg_uart0 = UART_DEFAULT_CONFIG();
    cfg_uart0.baudrate = 9600;
    cfg_uart0.remap=1;
    uart_init(UART_NUM_0,&cfg_uart0);
    return 0;
}

int sensor_init(){
	if (sensor_ctx.flag_init){
		return 0;
	}
	memset(&sensor_ctx, 0, sizeof(sensor_ctx_t));
	mb_init();
    uart_ds2480b();
	sensor_ctx.flag_init = 1;
	return 0;
}


int sensor_run(){
	uint32_t worktime = 0;
	modbus_run();
    dev_type_choose_function();
	worktime = worktime_get()/1000;
	if (sensor_ctx.flag_reset){
		SYS_ResetExecute();
	}
	if (worktime != sensor_ctx.worktime){
		sensor_ctx.worktime = worktime;
		modbus_reg_update(MB_REG_ADDR_WORKTIME_H, (worktime >> 16) & 0xffffU);
		modbus_reg_update(MB_REG_ADDR_WORKTIME_L, worktime & 0xffffU);
	}
	return 0;
}
