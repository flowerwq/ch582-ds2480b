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

    uart_config_t cfg_uart0 = UART_DEFAULT_CONFIG();
    cfg_uart0.baudrate = 9600;
    cfg_uart0.remap=1;
    uart_init(UART_NUM_0,&cfg_uart0);

	OLED_Init();
	OLED_ShowPicture(32, 0, 64, 64, (uint8_t *)smail_64x64_1, 1);
	OLED_Refresh();
	
	display_init();
	cfg_init();
	sensor_init();
    DS2480B_Detect();
    mdc02_range(0, 30);
    
    find_device();
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
        read_caps();
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

///*********************************************************************
// * @fn      UART1_IRQHandler
// *
// * @brief   UART1中断函数
// *
// * @return  none
// */
//__INTERRUPT
//__HIGH_CODE
//void UART1_IRQHandler(void)
//{
//    volatile uint8_t i;
//
//    switch(UART1_GetITFlag())
//    {
//        case UART_II_LINE_STAT: // 线路状态错误
//        {
//            UART1_GetLinSTA();
//            break;
//        }
//
//        case UART_II_RECV_RDY: // 数据达到设置触发点
//            for(i = 0; i != trigB; i++)
//            {
//                RxBuff[i] = UART1_RecvByte();
//                UART1_SendByte(RxBuff[i]);
//            }
//            break;
//
//        case UART_II_RECV_TOUT: // 接收超时，暂时一帧数据接收完成
//            i = UART1_RecvString(RxBuff);
//            UART1_SendString(RxBuff, i);
//            break;
//
//        case UART_II_THR_EMPTY: // 发送缓存区空，可继续发送
//            break;
//
//        case UART_II_MODEM_CHG: // 只支持串口0
//            break;
//
//        default:
//            break;
//    }
//}
