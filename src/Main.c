/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : 串口1收发演示
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/
#include "stdio.h"
#include "CH58x_common.h"
#include "worktime.h"
#include "configtool.h"
#include "oled.h"
#include "bmp.h"
#include "display.h"
#include "version.h"
#include "modbus.h"
#include "appinfo.h"
#include "uid.h"
#include "sensor.h"
#include "uart.h"
#include "ds2480b/commun.h"
#include "ds2480b/uart-ds2480.h"
#include "ds2480b/common.h"
#include "ds2480b/onewire.h"

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */

int uuid_dump(){
	uint8_t uuid[10] = {0};
	int i = 0;
	GET_UNIQUE_ID(uuid);
	PRINT("deviceid:");
	for (i = 0; i < 8; i++){
		PRINT("%02x", uuid[i]);
	}
	PRINT("\r\n");
	return 0;
}
int reset_dump(){
	SYS_ResetStaTypeDef rst = SYS_GetLastResetSta();
	PRINT("rst(");
	switch(rst){
		case RST_STATUS_SW:
			PRINT("sw reset");
			break;
		case RST_STATUS_RPOR:
			PRINT("poweron");
			break;
		case RST_STATUS_WTR:
			PRINT("wdt");
			break;
		case RST_STATUS_MR:
			PRINT("manual reset");
			break;
		case RST_STATUS_LRM0:
			PRINT("software wakeup");
			break;
		case RST_STATUS_GPWSM:
			PRINT("shutdown wakeup");
			break;
		case RST_STATUS_LRM1:
			PRINT("wdt wakeup");
			break;
		case RST_STATUS_LRM2:
			PRINT("manual wakeup");
			break;
	}
	PRINT(")\r\n");
	return 0;
}

int main()
{
    uint32_t appversion;
	worktime_t worktime = 0;
	char buf[DISPLAY_LINE_LEN + 1];
    SetSysClock(CLK_SOURCE_PLL_60MHz);
	worktime_init();
	
    /* 配置串口1： */
	uart_config_t cfg = UART_DEFAULT_CONFIG();
	uart_init(UART_NUM_1, &cfg);

	reset_dump();
	PRINT("app start ...\r\n");
	uuid_dump();

    

	OLED_Init();
	OLED_ShowPicture(32, 0, 64, 64, (uint8_t *)smail_64x64_1, 1);
	OLED_Refresh();
	
	display_init();
	cfg_init();
	sensor_init();

    DS2480B_Detect();    
    find_device_config();
	while(worktime_since(worktime) < 1000){
		__nop();
	}
	OLED_Clear();
	//DISPLAY_PRINT("Boot:%s", CURRENT_VERSION_STR());
//	if (upgrade_app_available()){
//		appversion = upgrade_app_version();
//		version_str(appversion, buf, DISPLAY_LINE_LEN);
//		DISPLAY_PRINT("APP:%s", buf);
//	}else{
//		DISPLAY_PRINT("APP:none");
//	}
	PRINT("main loop start ...\r\n");
    while(1){
		OLED_Refresh();
		
//		if (worktime_since(worktime) >= 1000){
//			worktime = worktime_get();
//			if ((worktime / 1000) % 2){
//				OLED_ShowPicture(32, 0, 64, 64, (uint8_t *)smail_64x64_1, 1);
//			}else{
//				OLED_ShowPicture(32, 0, 64, 64, (uint8_t *)smail_64x64_2, 1);
//			}
//		}
		sensor_run();
	}
}

