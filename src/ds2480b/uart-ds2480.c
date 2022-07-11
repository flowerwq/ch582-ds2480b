#include "ds2480b/commun.h"
#include "ds2480b/uart-ds2480.h"
#include "ds2480b/common.h"
#include "ds2480b/onewire.h"
#include "ds2480b/uart-ds2480.h"

/****偏置电容和反馈电容阵列权系数****/
static const float COS_Factor[8] = {0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 40.0};
/*Cos= (40.0*q[7]+32.0*q[6]+16.0*q[5]+8.0*q[4]+4.0*q[3]+2.0*q[2]+1.0*q[1]+0.5*q[0])*/
static const struct  {float Cfb0; float Factor[6];} CFB = { 2.0, 2.0, 4.0, 8.0, 16.0, 32.0, 46.0};
/*Cfb =(46*p[5]+32*p[4]+16*p[3]+8*p[2]+4*p[1]+2*p[0]+2)*/

static volatile int ow_flag_timerout = 0;
extern unsigned char ROM_NO[8];
onewire onewire_dev;

bool send_matchrom()
{
    unsigned char sendpacket[10] = {0};
    int sendlen = 1;
	if(!OWReset()){ 			//检查总线上从机是否存在
		return FALSE;
	}

// select the device
     sendpacket[0] = 0x55; // match command
//     for (int i = 0; i < 8; i++){
//        sendpacket[i+1] = ROM_NO[i];
//     }
    sendpacket[sendlen++] = 0x28;
    sendpacket[sendlen++] = 0x75;
    sendpacket[sendlen++] = 0x00;
    sendpacket[sendlen++] = 0x76;
    sendpacket[sendlen++] = 0x44;
    sendpacket[sendlen++] = 0xf5;
    sendpacket[sendlen++] = 0x00;
    sendpacket[sendlen++] = 0x00;
     OWBlock(sendpacket,9);
	 return TRUE;

}

static bool  mdc02_readscratchpad_skiprom(uint8_t *scr)
{
    int16_t i;
//    unsigned char sendpacket[10] = {0};
//    int sendlen;

	send_matchrom();
//    sendlen = 0;
//    sendpacket[sendlen++] = 0xBE;
//    for (i = 0; i < 9; i++){
//       sendpacket[sendlen++] = 0xFF;
//    }
//
//    if (OWBlock(sendpacket,sendlen))
//    {
//       PRINT("Scatchpad result = ");
//       for (i = 0; i < sendlen; i++){
//          PRINT("%02X",sendpacket[i]);
//       }
//       PRINT("\n");
//    }
//    else{
//       PRINT("ERROR\n");
//    }

    OWWriteByte(READ_SCRATCHPAD);//读取包含配置寄存器在内的所有暂存器内容命令

	for(i=0; i < sizeof(MDC02_SCRATCHPAD_READ); i++)
    {
		*scr++ = OWReadByte();
	}

    return TRUE;
}

bool mdc02_readc2c3c4_skiprom(uint8_t *scr)
{
    uint16_t i;
	send_matchrom();

    OWWriteByte(READ_C2C3C4);//读取电容通道 2、 3 和 4 命令

	for(i=0; i < sizeof(MDC02_C2C3C4); i++)
   {
	    *scr++ = OWReadByte();
	}

    return TRUE;
}

static bool read_tempwaiting(uint16_t *iTemp)
{
	uint8_t scrb[sizeof(MDC02_SCRATCHPAD_READ)];
	MDC02_SCRATCHPAD_READ *scr = (MDC02_SCRATCHPAD_READ *) scrb;

	/*读9个字节。前两个是温度转换结果，最后字节是前8个的校验和--CRC。*/
	if(mdc02_readscratchpad_skiprom(scrb) == FALSE)
	{
		return FALSE;  /*读寄存器失败*/
	}

	/*计算接收的前8个字节的校验和，并与接收的第9个CRC字节比较。*/
    if(scrb[8] != onewire_crc8(&scrb[0], 8))
    {
    	return FALSE;  /*CRC验证未通过*/
    }

	/*将温度测量结果的两个字节合成为16位字。*/
    *iTemp=(uint16_t)scr->T_msb<<8 | scr->T_lsb;

    return TRUE;
}
static bool converttemp()
{
	send_matchrom();
	OWWriteByte(CONVERT_T);

  return TRUE;
}
//多个字节的crc8算法
uint8_t onewire_crc8(uint8_t *serial, uint8_t length)
{
    uint8_t crc8 = 0x00;
    uint8_t pDataBuf;
    uint8_t i;

    while(length--) {
//        pDataBuf = *serial++;
        crc8 ^= *serial++;
        for(i=0; i<8; i++) {
            if((crc8^(pDataBuf))&0x01) {
                crc8 ^= 0x18;
                crc8 >>= 1;
                crc8 |= 0x80;
            }
            else {
                crc8 >>= 1;
            }
            pDataBuf >>= 1;
    }
  }
    return crc8;
}

static float MY18B20_outputtotemp(int16_t out)
{
	return ((float)out/16.0);	
}
float mdc02_outputtocap(uint16_t out, float Co, float Cr)
{
	return (2.0*(out/65535.0-0.5)*Cr+Co);
}


int read_tmp(){
    float fTemp=0; 
	uint16_t iTemp=0; 
	
	if(converttemp() == TRUE)
	{
    	DelayUs(15);
		read_tempwaiting(&iTemp);
		fTemp=MY18B20_outputtotemp((int16_t)iTemp);
		PRINT("T= %3.3f \r\n", fTemp);
		PRINT("\r\n");
	}
	else
	{
		PRINT("\r\n No MDC02");
	}
//    WWDG_SetCounter(0);
	DelayUs(990);
//    WWDG_SetCounter(0);
				
	return 1;
}

static bool convertcap()
 {
     send_matchrom();
    //WWDG_SetCounter(0);
 
     OWWriteByte(CONVERT_C);//启动电容转换
     return TRUE;
 }
static bool convert_tempcap1()
 {
     send_matchrom();
    //WWDG_SetCounter(0);
 
     OWWriteByte(CONVERT_TC1);
 
   return TRUE;
 }
 ///**
 //  * @brief  读状态和配置
 //  * @param  status 返回的状态寄存器值
 //  * @param  cfg 返回的配置寄存器值
 //  * @retval 状态
 //*/
 bool readstatusconfig(uint8_t *status, uint8_t *cfg)
 {
     uint8_t scrb[sizeof(MDC02_SCRATCHPAD_READ)];
     MDC02_SCRATCHPAD_READ *scr = (MDC02_SCRATCHPAD_READ *) scrb;
 
     /*读9个字节。第7字节是系统配置寄存器，第8字节是系统状态寄存器。最后字节是前8个的校验和--CRC。*/
     if(mdc02_readscratchpad_skiprom(scrb) == FALSE)
     {
         return FALSE;  /*CRC验证未通过*/
     }
 
     /*计算接收的前8个字节的校验和，并与接收的第9个CRC字节比较。*/
     if(scrb[8] != onewire_crc8(&scrb[0], 8))
     {
         return FALSE;  /*CRC验证未通过*/
     }
 
     *status = scr->Status;
     *cfg = scr->Cfg;
 
     return TRUE;
 }
 float cfbcfgtocaprange(uint8_t fbCfg)
{
	uint8_t i;
	float Crange = CFB.Cfb0;

	for(i = 0; i <= 5; i++)
	{
		if(fbCfg & 0x01){
			Crange += CFB.Factor[i];
		}
        fbCfg >>= 1;
	}
	return (0.507/3.6) * Crange;
}
 bool readcfbconfig(uint8_t *Cfb)
{
	uint8_t scrb[sizeof(MDC02_SCRPARAMETERS)];
	MDC02_SCRPARAMETERS *scr = (MDC02_SCRPARAMETERS *) scrb;

	/*读15个字节。第5字节是偏置电容配置寄存器，第10字节是量程电容配置寄存器，最后字节是前14个的校验和--CRC。*/
	if(mdc02_readparameters_skiprom(scrb) == FALSE)
	{
		return FALSE;  /*读寄存器失败*/
	}

	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/
	if(scrb[sizeof(MDC02_SCRPARAMETERS)-1] != onewire_crc8(&scrb[0], sizeof(MDC02_SCRPARAMETERS)-1))
	{
		return FALSE;   /*CRC验证未通过*/
	}

	*Cfb = scr->Cfb & MDC02_CFEED_CFB_MASK;
//	PRINT("read register cfb:%d\r\n",*Cfb);
	return TRUE;;
}

 /////**
////  * @brief  获取配置的量程电容数值（pF）
////  * @param  Crange：返回量程电容数值
////  * @retval 无
////*/
void getcfg_caprange(float *Crange)
{
	uint8_t Cfb_cfg;

	readcfbconfig(&Cfb_cfg);
	*Crange = cfbcfgtocaprange(Cfb_cfg);
	PRINT("cfbcfg to capcfb:%.4f\r\n",*Crange);
}
//
///* @brief  获取配置的偏置电容数值（pF）
//  * @param  Coffset：偏置电容配置
//  * @retval 无
//*/
void getcfg_capoffset(float *Coffset)
{	uint8_t Cos_cfg;

	readcosconfig(&Cos_cfg);
	*Coffset = coscfgtocapoffset(Cos_cfg);
	PRINT("coscfg to capoffset:%.2f\r\n",*Coffset);
}
 /**
  * @brief  读电容配置
  * @param  Coffset：配置的偏置电容。
  * @param  Crange：配置的量程电容。
  * @retval 无
*/
bool readcosconfig(uint8_t *Coscfg)
{
	uint8_t scrb[sizeof(MDC02_SCRPARAMETERS)];
	MDC02_SCRPARAMETERS *scr = (MDC02_SCRPARAMETERS *) scrb;

	/*读15个字节。第5字节是偏置电容配置寄存器，第10字节是量程电容配置寄存器，最后字节是前14个的校验和--CRC。*/
	if(mdc02_readparameters_skiprom(scrb) == FALSE)
	{
		return FALSE;  /*读寄存器失败*/
	}

	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/
  	if(scrb[sizeof(MDC02_SCRPARAMETERS)-1] != onewire_crc8(&scrb[0], sizeof(MDC02_SCRPARAMETERS)-1))
  	{
  		return FALSE;  /*CRC验证未通过*/
  	}

	*Coscfg = scr->Cos & (0xFF >> (3 - (scr->Cfb >> 6))); //屏蔽掉无效位，根据CFB寄存器的高2位
//	PRINT("read register cos:%d\r\n",*Coscfg);

  	return TRUE;
}
float coscfgtocapoffset(uint8_t osCfg)
{
	uint8_t i;
	float Coffset = 0.0;

	for(i = 0; i < 8; i++)
	{
		if(osCfg & 0x01){
		    Coffset += COS_Factor[i];
        }
        osCfg >>= 1;
		
	}

	return Coffset;
}

bool readcapconfigure(float *Coffset, float *Crange)
{
	getcfg_capoffset(Coffset);
	getcfg_caprange(Crange);

	return TRUE;
}
 bool readtempcap1(uint16_t *iTemp, uint16_t *iCap1)
 {
     uint8_t scrb[sizeof(MDC02_SCRATCHPAD_READ)];
     MDC02_SCRATCHPAD_READ *scr = (MDC02_SCRATCHPAD_READ *) scrb;
 
     /*读9个字节。前两个是温度转换结果，最后字节是前8个的校验和--CRC。*/
     if(mdc02_readscratchpad_skiprom(scrb) == FALSE)
     {
         return FALSE;  /*读寄存器失败*/
     }
    //WWDG_SetCounter(0);
     /*计算接收的前8个字节的校验和，并与接收的第9个CRC字节比较。*/
     if(scrb[8] != onewire_crc8(&scrb[0], 8))
     {
         return FALSE;  /*CRC验证未通过*/
     }
     
     *iTemp=(uint16_t)scr->T_msb<<8 | scr->T_lsb;
     *iCap1=(uint16_t)scr->C1_msb<<8 | scr->C1_lsb;
     return TRUE;
 }
 
 bool readcapc2c3c4(uint16_t *iCap)
 {
     uint8_t scrb[sizeof(MDC02_C2C3C4)];
     MDC02_C2C3C4 *scr = (MDC02_C2C3C4 *) scrb;
 
     /*读6个字节。每两个字节依序分别为通道2、3和4的测量结果，最后字节是前两个的校验和--CRC。*/
     if(mdc02_readc2c3c4_skiprom(scrb) == FALSE)
     {
         return FALSE;  /*读寄存器失败*/
     }
    //WWDG_SetCounter(0);
     /*计算接收的前两个字节的校验和，并与接收的第3个CRC字节比较。*/
 //  if(scrb[3] != onewire_crc8(scrb, 2))
 //  {
 //      return FALSE;  /*CRC验证未通过*/
 //  }
 
     *iCap= (uint16_t)scr->C2_msb<<8 | scr->C2_lsb;
     PRINT("%.2x\r\n",*iCap);
 //  iCap[1] = (uint16_t)scr->C3_msb<<8 | scr->C3_lsb;
 //  iCap[2] = (uint16_t)scr->C4_msb<<8 | scr->C4_lsb;
     return TRUE;
 }

 static int judge_status(float a,float b){
     if(a > 5.0 && b > 5.0){
         PRINT("进水了\r\n");
         return 1;
     }
     else{
         return 0;
     }
 }
 bool writecfbconfig(uint8_t Cfb)
{
	uint8_t scrb[sizeof(MDC02_SCRPARAMETERS)];
	MDC02_SCRPARAMETERS *scr = (MDC02_SCRPARAMETERS *) scrb;

	/*读15个字节。第5字节是偏置电容配置寄存器，第10字节是量程电容配置寄存器，最后字节是前14个的校验和--CRC。*/
	if(mdc02_readparameters_skiprom(scrb) == FALSE)
	{
		return FALSE;   /*读寄存器失败*/
	}

	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/
  	if(scrb[sizeof(MDC02_SCRPARAMETERS)-1] != onewire_crc8(&scrb[0], sizeof(MDC02_SCRPARAMETERS)-1))
  	{
  		return FALSE;  /*CRC验证未通过*/
	}

	scr->Cfb &= ~CFB_CFBSEL_Mask;
	scr->Cfb |= Cfb;
//	PRINT("write cfb:%d\r\n",scr->Cfb);

	mdc02_writeparameters_skiprom(scrb);
	return TRUE;
}
 bool writecosconfig(uint8_t Coffset, uint8_t Cosbits)
{
	uint8_t scrb[sizeof(MDC02_SCRPARAMETERS)];
	MDC02_SCRPARAMETERS *scr = (MDC02_SCRPARAMETERS *) scrb;

	/*读15个字节。第5字节是偏置电容配置寄存器，第10字节是量程电容配置寄存器，最后字节是前14个的校验和--CRC。*/
	if(mdc02_readparameters_skiprom(scrb) == FALSE)
	{
		return FALSE;   /*读寄存器失败*/
	}

	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/
  	if(scrb[sizeof(MDC02_SCRPARAMETERS)-1] != onewire_crc8(&scrb[0], sizeof(MDC02_SCRPARAMETERS)-1))
  	{
		return FALSE;  /*CRC验证未通过*/
  	}

	scr->Cos = Coffset;
	scr->Cfb = (scr->Cfb & ~CFB_COSRANGE_Mask) | Cosbits;
//	PRINT("write cosconfig:%d %f\r\n",scr->Cos,scr->Cfb);

	mdc02_writeparameters_skiprom(scrb);

  	return TRUE;
}
 bool mdc02_capconfigureoffset(float Coffset)
{
	uint8_t CosCfg, Cosbits;
	float b=Coffset+0.25;
	CosCfg = captocoscfg(b);

	if(!(CosCfg & ~0x1F)) {
		Cosbits = COS_RANGE_5BIT;
	}
	else if(!(CosCfg & ~0x3F)) {
		Cosbits = COS_RANGE_6BIT;
	}
	else if(!(CosCfg & ~0x7F)){
		Cosbits = COS_RANGE_7BIT;
	}
	else{
		Cosbits = COS_RANGE_8BIT;
	}
//	PRINT("%d  %d\r\n",CosCfg,Cosbits);
	writecosconfig(CosCfg, Cosbits);

	return TRUE;
}
 uint8_t captocoscfg(float osCap)
{
	int i; 
	uint8_t CosCfg = 0x00;

	for(i = 7; i >= 0; i--)
	{
		if(osCap >= COS_Factor[i])
		{
			CosCfg |= (0x01 << i);
			osCap -= COS_Factor[i];
		}
	}
	return CosCfg;
}
 uint8_t caprangetocfbcfg(float fsCap)
{
	int8_t i; 
	uint8_t CfbCfg = 0x00;

	fsCap = fsCap * (3.6/0.507);

	fsCap -= CFB.Cfb0;

	for(i = 5; i >= 0; i--)
	{
		if(fsCap >= CFB.Factor[i])
		{
			fsCap -= CFB.Factor[i];
			CfbCfg |= (0x01 << i);
		}
	}

	return CfbCfg;
}
 bool mdc02_capconfigurefs(float Cfs)
{
	uint8_t Cfbcfg;

	Cfs = (Cfs + 0.1408);
	Cfbcfg = caprangetocfbcfg(Cfs);
//	PRINT("before write cfbcfg: %d\r\n",Cfbcfg);

	writecfbconfig(Cfbcfg);

	return TRUE;
}
 bool mdc02_capconfigurerange(float Cmin, float Cmax)
{ 
	float Cfs, Cos;

//	if(!((Cmax <= 119.0) && (Cmax > Cmin) && (Cmin >= 0.0) && ((Cmax-Cmin) <= 31.0)))
//	return FALSE;	//The input value is out of range.

	Cos = (Cmin + Cmax)/2.0;
	Cfs = (Cmax - Cmin)/2.0;

	mdc02_capconfigureoffset(Cos);
	mdc02_capconfigurefs(Cfs);

	return TRUE;
}
 int mdc02_range(float Cmin,float Cmax)
 { 
 //      printf("\r\nCmin= %3.2f Cmax=%3.2f", Cmin, Cmax);
     if(!((Cmax <= 119.0) && (Cmax > Cmin) && (Cmin >= 0.0) && 
             ((Cmax-Cmin) <= 31.0)))  
     {
         PRINT(" %s", "The input is out of range"); 
         return 0;
     }
         
     mdc02_capconfigurerange(Cmin, Cmax);
         
     readcapconfigure(&onewire_dev.CapCfg_offset, &onewire_dev.CapCfg_range);
 //  PRINT("%.2f  %.5f\r\n",CapCfg_offset,CapCfg_range);
         
     return 1;
 }

 int read_caps()
{	
	float fcap1, fcap2, fcap3, fcap4; uint16_t iTemp, icap1, icap[1];
	uint8_t status, cfg;
	uint8_t status_wids=0;
	
	setcapchannel(CAP_CH1CH2_SEL);
   //WWDG_SetCounter(0);
	readstatusconfig((uint8_t *)&status, (uint8_t *)&cfg);
   //WWDG_SetCounter(0);
	
	if(convertcap() == FALSE)
	{
		PRINT("No MDC02\r\n");
	}
	else{
//		WWDG_SetCounter(0 );
		DelayUs(15);
		readcapconfigure(&onewire_dev.CapCfg_offset, &onewire_dev.CapCfg_range);
       //WWDG_SetCounter(0);
//		PRINT("%f  %.5f\r\n",onewire_dev.CapCfg_offset,onewire_dev.CapCfg_range);
		readstatusconfig((uint8_t *)&status, (uint8_t *)&cfg);
       //WWDG_SetCounter(0);

		readtempcap1(&iTemp, &icap1);
       //WWDG_SetCounter(0);
		if(readcapc2c3c4(&icap[0]) == FALSE){
			PRINT("read cap2 error");
		}
       //WWDG_SetCounter(0);
		fcap1 = mdc02_outputtocap(icap1, onewire_dev.CapCfg_offset, onewire_dev.CapCfg_range);
		fcap2 = mdc02_outputtocap(icap[0], onewire_dev.CapCfg_offset, onewire_dev.CapCfg_range);
       //WWDG_SetCounter(0);
		PRINT("C1=%4d , %6.3f pf  ,C2=%4d, %6.3f pf , S=%02X   C=%02X\r\n", icap1, fcap1, icap[0], fcap2, status, cfg);

		status_wids=judge_status(fcap1,fcap2);
		PRINT("\r\n");
//		OLED_ShowNum(82, 16, status_wids, 1, 16, 1);
//
//		OLED_Refresh(0);
//		modbus_di_update(MB_DI_ADDR_WIDS,status_wids);
		
		}
//    WWDG_SetCounter(0);

	DelayUs(990);
//    WWDG_SetCounter(0);
	return 1;		
}

bool mdc02_writeparameters_skiprom(uint8_t *scr)
{
    uint16_t i;

	send_matchrom();

    OWWriteByte(WRITE_PARAMETERS);

	for(i=0; i < sizeof(MDC02_SCRPARAMETERS); i++)
    {
    	OWWriteByte(*scr++);
	}

    return TRUE;
}
bool mdc02_readparameters_skiprom(uint8_t *scr)
{
    uint16_t i;
	send_matchrom();
    OWWriteByte(READ_PARAMETERS );

	for(i=0; i < sizeof(MDC02_SCRPARAMETERS); i++)
    {
    	*scr++ = OWReadByte();
	}

    return TRUE;
}

bool setcapchannel(uint8_t channel)
{
	uint8_t scrb[sizeof(MDC02_SCRPARAMETERS)];
	MDC02_SCRPARAMETERS *scr = (MDC02_SCRPARAMETERS *) scrb;

	/*读15个字节。第4字节是通道选择寄存器，最后字节是前14个的校验和--CRC。*/
	if(mdc02_readparameters_skiprom(scrb) == FALSE)
	{
		return FALSE;  /*读寄存器失败*/
	}

	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/
	if(scrb[sizeof(MDC02_SCRPARAMETERS)-1] != onewire_crc8(&scrb[0], sizeof(MDC02_SCRPARAMETERS)-1))
	{
		return FALSE;  /*CRC验证未通过*/
	}

	scr->Ch_Sel = (scr->Ch_Sel & ~CCS_CHANNEL_MASK) | (channel & CCS_CHANNEL_MASK);

	mdc02_writeparameters_skiprom(scrb);

	return TRUE;
}

static int read_tempc1()
{
	uint16_t iTemp, iCap1; 
	float fTemp, fCap1;
		
	readcapconfigure(&onewire_dev.CapCfg_offset, &onewire_dev.CapCfg_range);
	setcapchannel(CAP_CH1_SEL);

	if(convert_tempcap1() == TRUE)
	{
		DelayUs(15);		
		if(readtempcap1(&iTemp, &iCap1) == TRUE) 
		{	
//			PRINT("%2x\r\n",iTemp);
			fTemp=mdc02_outputtotemp(iTemp);
			fCap1=mdc02_outputtocap(iCap1, onewire_dev.CapCfg_offset, onewire_dev.CapCfg_range);
           //WWDG_SetCounter(0);
			PRINT("T= %3.3f ℃ C1= %6.3f pF\r\n", fTemp, fCap1);
		}
	}
	else
	{
		PRINT("\r\n No MDC02");
	}
//    WWDG_SetCounter(0);
	DelayUs(990);
//    WWDG_SetCounter(0);
	return 1;		
}

///*********************************************************************
// * @fn      TMR0_IRQHandler
// *
// * @brief   TMR0中断函数
// *
// * @return  none
// */
//__INTERRUPT
//__HIGH_CODE
//void TMR1_IRQHandler(void) // TMR1 定时中断
//{
//    if(TMR1_GetITFlag(TMR0_3_IT_CYC_END))
//    {
//    	TMR1_Disable();
//        TMR1_ClearITFlag(TMR0_3_IT_CYC_END); // 清除中断标志
//        ow_flag_timerout = 1;
//    }
//}
//
//
//static int ow_timer_init(){
//	TMR1_ITCfg(ENABLE, TMR0_3_IT_CYC_END); // 开启中断
//	PFIC_EnableIRQ(TMR1_IRQn);
//	return 0;
//}
//
//static int ow_timer_start(uint32_t timeout_us){
//	ow_flag_timerout = 0;
//	TMR1_TimerInit(GetSysClock() / 1000000  * timeout_us);
//	return 0;
//}
//
//static int ow_timer_stop(){
//	TMR1_Disable();
//	return 0;
//}
//
//static int DelayUs(uint32_t us){
//	ow_timer_start(us);
//	while(!ow_flag_timerout);
//	ow_timer_stop();
//	return 0;
//}
//



//
////
/////********    convert CAP       ********/////////////

/**
  * @brief  读电容配置
  * @param  Coffset：配置的偏置电容。
  * @param  Crange：配置的量程电容。
  * @retval 无
*/

//bool readcapconfigure(float *Coffset, float *Crange,int num)
//{
//	getcfg_capoffset(Coffset,num);
//	getcfg_caprange(Crange,num);
//
//	return TRUE;
//}
//////
///////**
//////  * @brief  获取配置的量程电容数值（pF）
//////  * @param  Crange：返回量程电容数值
//////  * @retval 无
//////*/
//void getcfg_caprange(float *Crange,int num)
//{
//	uint8_t Cfb_cfg;
//
//	readcfbconfig(&Cfb_cfg,num);
//	*Crange = cfbcfgtocaprange(Cfb_cfg);
////	PRINT("cfbcfg to capcfb:%.4f\r\n",*Crange);
//}
////
/////* @brief  获取配置的偏置电容数值（pF）
////  * @param  Coffset：偏置电容配置
////  * @retval 无
////*/
//void getcfg_capoffset(float *Coffset,int num)
//{	uint8_t Cos_cfg;
//
//	readcosconfig(&Cos_cfg,num);
//	*Coffset = coscfgtocapoffset(Cos_cfg);
////	PRINT("coscfg to capoffset:%.2f\r\n",*Coffset);
//}
////
//////
///////**
//////  * @brief  将量程电容配置转换为对应的量程电容数值（pF）
//////  * @param  fbCfg：量程电容配置
//////  * @retval 对应量程电容的数值
//////*/
//float cfbcfgtocaprange(uint8_t fbCfg)
//{
//	uint8_t i;
//	float Crange = CFB.Cfb0;
//
//	for(i = 0; i <= 5; i++)
//	{
//		if(fbCfg & 0x01){
//			Crange += CFB.Factor[i];
//		}
//		fbCfg >>= 1;
//	}
//	return (0.507/3.6) * Crange;
//}
////
///////**
//////  * @brief  读量程电容配置寄存器内容
//////  * @param  Cfb：量程配置寄存器低6位的内容
//////  * @retval 状态
//////*/
//bool readcfbconfig(uint8_t *Cfb,int num)
//{
//	uint8_t scrb[sizeof(MDC02_SCRPARAMETERS)];
//	MDC02_SCRPARAMETERS *scr = (MDC02_SCRPARAMETERS *) scrb;
//
//	/*读15个字节。第5字节是偏置电容配置寄存器，第10字节是量程电容配置寄存器，最后字节是前14个的校验和--CRC。*/
//	if(mdc02_readparameters_skiprom(scrb,num) == FALSE)
//	{
//		return FALSE;  /*读寄存器失败*/
//	}
//
//	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/
//	if(scrb[sizeof(MDC02_SCRPARAMETERS)-1] != onewire_crc8(&scrb[0], sizeof(MDC02_SCRPARAMETERS)-1))
//	{
//		return FALSE;   /*CRC验证未通过*/
//	}
//
//	*Cfb = scr->Cfb & MDC02_CFEED_CFB_MASK;
////	PRINT("read register cfb:%d\r\n",*Cfb);
//	return TRUE;;
//}
////
////
/////**
////  * @brief  将偏置电容配置转换为对应的偏置电容数值（pF）
////  * @param  osCfg：偏置电容配置
////  * @retval 对应偏置电容的数值
////*/
//float coscfgtocapoffset(uint8_t osCfg)
//{
//	uint8_t i;
//	float Coffset = 0.0;
//
//	for(i = 0; i < 8; i++)
//	{
//		if(osCfg & 0x01) Coffset += COS_Factor[i];{
//			osCfg >>= 1;
//		}
//	}
//
//	return Coffset;
//}
//
////
//////
///**
//  * @brief  读偏置电容配置寄存器内容
//  * @param  Coffset：偏置配置寄存器有效位的内容
//  * @retval 无
//*/
//
//bool readcosconfig(uint8_t *Coscfg,int num)
//{
//	uint8_t scrb[sizeof(MDC02_SCRPARAMETERS)];
//	MDC02_SCRPARAMETERS *scr = (MDC02_SCRPARAMETERS *) scrb;
//
//	/*读15个字节。第5字节是偏置电容配置寄存器，第10字节是量程电容配置寄存器，最后字节是前14个的校验和--CRC。*/
//	if(mdc02_readparameters_skiprom(scrb,num) == FALSE)
//	{
//		return FALSE;  /*读寄存器失败*/
//	}
//
//	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/
//  	if(scrb[sizeof(MDC02_SCRPARAMETERS)-1] != onewire_crc8(&scrb[0], sizeof(MDC02_SCRPARAMETERS)-1))
//  	{
//  		return FALSE;  /*CRC验证未通过*/
//  	}
//
//	*Coscfg = scr->Cos & (0xFF >> (3 - (scr->Cfb >> 6))); //屏蔽掉无效位，根据CFB寄存器的高2位
////	PRINT("read register cos:%d\r\n",*Coscfg);
//
//  	return TRUE;
//}
//////
//bool mdc02_writeparameters_skiprom(uint8_t *scr, int num)
//{
//    int16_t i;
//
////    if(onewire_resetcheck() != 0){
////		return FALSE;
////	}
////
////    read_command();
//	send_matchrom(device_t.ROMID,num);
//
//    onewire_writebyte(WRITE_PARAMETERS);
//
//	for(i=0; i < sizeof(MDC02_SCRPARAMETERS); i++)
//    {
//    	onewire_writebyte(*scr++);
//	}
//
//    return TRUE;
//}
//bool mdc02_readparameters_skiprom(uint8_t *scr,int num)
//{
//    int16_t i;
////
////    if(onewire_resetcheck() != 0){
////		return FALSE;
////	}
////
////    read_command();
//	send_matchrom(device_t.ROMID,num);
//    onewire_writebyte(READ_PARAMETERS );
//
//	for(i=0; i < sizeof(MDC02_SCRPARAMETERS); i++)
//    {
//    	*scr++ = onewire_read();
//	}
//
//    return TRUE;
//}
//
/////**
////  * @brief  读状态和配置
////  * @param  status 返回的状态寄存器值
////  * @param  cfg 返回的配置寄存器值
////  * @retval 状态
////*/
//bool readstatusconfig(uint8_t *status, uint8_t *cfg,int num)
//{
//	uint8_t scrb[sizeof(MDC02_SCRATCHPAD_READ)];
//	MDC02_SCRATCHPAD_READ *scr = (MDC02_SCRATCHPAD_READ *) scrb;
//
//	/*读9个字节。第7字节是系统配置寄存器，第8字节是系统状态寄存器。最后字节是前8个的校验和--CRC。*/
//	if(mdc02_readscratchpad_skiprom(scrb,num) == FALSE)
//	{
//		return FALSE;  /*CRC验证未通过*/
//	}
//
//	/*计算接收的前8个字节的校验和，并与接收的第9个CRC字节比较。*/
//  	if(scrb[8] != onewire_crc8(&scrb[0], 8))
//  	{
//  		return FALSE;  /*CRC验证未通过*/
//  	}
//
//	*status = scr->Status;
//	*cfg = scr->Cfg;
//
//	return TRUE;
//}
////
//////
///////**
//////  * @brief  读芯片寄存器的暂存器组
//////  * @param  scr：字节数组指针， 长度为 @sizeof（MDC04_SCRATCHPAD_READ）
//////  * @retval 读状态
//////*/
//
//bool mdc02_writescratchpadext_skiprom(uint8_t *scr)
//{
//    int16_t i;
//
//    if(onewire_resetcheck() != 0)
//			return FALSE;
//	read_command();
//    onewire_writebyte(0x77);
//
//	for(i=0; i<sizeof(MDC02_SCRATCHPADEXT)-1; i++)
//	{
//		onewire_writebyte(*scr++);
//	}
//
//    return TRUE;
//}
///**
//  * @brief  写芯片寄存器的暂存器组
//  * @param  scr：字节数组指针， 长度为 @sizeof（MDC04_SCRATCHPAD_WRITE）
//  * @retval 写状态
//**/
//bool mdc02_writescratchpad_skiprom(uint8_t *scr,int num)
//{
//    int16_t i;
//
//  	send_matchrom(device_t.ROMID, num);
//    onewire_writebyte(WRITE_SCRATCHPAD);
//
//	for(i=0; i < sizeof(MDC02_SCRATCHPAD_WRITE); i++)
//	{
//		onewire_writebyte(*scr++);
//	}
//
//    return TRUE;
//}
//
//
///**
//  * @brief  设置周期测量频率和重复性
//  * @param  mps 要设置的周期测量频率（每秒测量次数），可能为下列其一
//	*				@arg CFG_MPS_Single		：每执行ConvertTemp一次，启动一次温度测量
//	*				@arg CFG_MPS_Half			：每执行ConvertTemp一次，启动每秒0.5次重复测量
//	*				@arg CFG_MPS_1				：每执行ConvertTemp一次，启动每秒1次重复测量
//	*				@arg CFG_MPS_2				：每执行ConvertTemp一次，启动每秒2次重复测量
//	*				@arg CFG_MPS_4				：每执行ConvertTemp一次，启动每秒4次重复测量
//	*				@arg CFG_MPS_10				：每执行ConvertTemp一次，启动每秒10次重复测量
//  * @param  repeatability：要设置的重复性值，可能为下列其一
//	*				@arg CFG_Repeatbility_Low				：设置低重复性
//	*				@arg CFG_Repeatbility_Medium		：设置中重复性
//	*				@arg CFG_Repeatbility_High			：设置高重复性
//  * @retval 无
//*/
//bool setconfig(uint8_t mps, uint8_t repeatability,int num)
//{
//	uint8_t scrb[sizeof(MDC02_SCRATCHPAD_READ)];
//	MDC02_SCRATCHPAD_READ *scr = (MDC02_SCRATCHPAD_READ *) scrb;
//
//	/*读9个字节。第7字节是系统配置寄存器，第8字节是系统状态寄存器。最后字节是前8个的校验和--CRC。*/
//	if(mdc02_readscratchpad_skiprom(scrb,num) == FALSE)
//	{
//		return FALSE;  /*读暂存器组水平*/
//	}
//
//	/*计算接收的前8个字节的校验和，并与接收的第9个CRC字节比较。*/
//  if(scrb[8] != onewire_crc8(&scrb[0], 8))
//  {
//		return FALSE;  /*CRC验证未通过*/
//  }
//
//	scr->Cfg &= ~CFG_Repeatbility_Mask;
//	scr->Cfg |= repeatability;
//	scr->Cfg &= ~CFG_MPS_Mask;
//	scr->Cfg |= mps;
//
//	mdc02_writescratchpad_skiprom(scrb+4,num);
//
//	return TRUE;
//}
//
//////
//////
//bool setcapchannel(uint8_t channel,int num)
//{
//	uint8_t scrb[sizeof(MDC02_SCRPARAMETERS)];
//	MDC02_SCRPARAMETERS *scr = (MDC02_SCRPARAMETERS *) scrb;
//
//	/*读15个字节。第4字节是通道选择寄存器，最后字节是前14个的校验和--CRC。*/
//	if(mdc02_readparameters_skiprom(scrb,num) == FALSE)
//	{
//		return FALSE;  /*读寄存器失败*/
//	}
//
//	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/
//	if(scrb[sizeof(MDC02_SCRPARAMETERS)-1] != onewire_crc8(&scrb[0], sizeof(MDC02_SCRPARAMETERS)-1))
//	{
//		return FALSE;  /*CRC验证未通过*/
//	}
//
//	scr->Ch_Sel = (scr->Ch_Sel & ~CCS_CHANNEL_MASK) | (channel & CCS_CHANNEL_MASK);
//
//	mdc02_writeparameters_skiprom(scrb,num);
//
//	return TRUE;
//}
//
//bool convertcap(int num)
//{
////	if(onewire_resetcheck() !=  0){
////		return FALSE;
////    }
////
////    read_command();
//	send_matchrom(device_t.ROMID,num);
//   //WWDG_SetCounter(0);
//
//    onewire_writebyte(CONVERT_C);//启动电容转换
//    return TRUE;
//}
///**
//  * @brief  启动温度测量
//  * @param  无
//  * @retval 单总线发送状态
//*/
//bool converttemp(int num)
//{
//	send_matchrom(device_t.ROMID,num);
//	onewire_writebyte(CONVERT_T);
//
//  return TRUE;
//}
//
///**
//  * @brief  启动温度和电容通道1同时测量
//  * @param  无
//  * @retval 单总线发送状态
//*/
//bool convert_tempcap1(int num)
//{
////	if(onewire_resetcheck() != 0){
////		return FALSE;
////	}
//	
////	read_command();
//	send_matchrom(device_t.ROMID,num);
//   //WWDG_SetCounter(0);
//
//	onewire_writebyte(CONVERT_TC1);
//
//  return TRUE;
//}
//
//
//bool readtempcap1(uint16_t *iTemp, uint16_t *iCap1,int num)
//{
//	uint8_t scrb[sizeof(MDC02_SCRATCHPAD_READ)];
//	MDC02_SCRATCHPAD_READ *scr = (MDC02_SCRATCHPAD_READ *) scrb;
//
//	/*读9个字节。前两个是温度转换结果，最后字节是前8个的校验和--CRC。*/
//	if(mdc02_readscratchpad_skiprom(scrb,num) == FALSE)
//	{
//		return FALSE;  /*读寄存器失败*/
//	}
//   //WWDG_SetCounter(0);
//	/*计算接收的前8个字节的校验和，并与接收的第9个CRC字节比较。*/
//	if(scrb[8] != onewire_crc8(&scrb[0], 8))
//	{
//	    return FALSE;  /*CRC验证未通过*/
//	}
//	
//	*iTemp=(uint16_t)scr->T_msb<<8 | scr->T_lsb;
//	*iCap1=(uint16_t)scr->C1_msb<<8 | scr->C1_lsb;
//	return TRUE;
//}
//////
///////**
//////  * @brief  读电容通道2，1
//////  * @param  scr：字节数组指针， 长度为 @sizeof（MDC04_C2C3C4）
//////  * @retval 写状态
//////**/
//
//bool  mdc02_readscratchpad_skiprom(uint8_t *scr,int num)
//{
//    int16_t i;
//
//	/*size < sizeof(MDC04_SCRATCHPAD_READ)*/
////    if(onewire_resetcheck() != 0){
////		return FALSE;
////	}
////
////    read_command();
//	send_matchrom(device_t.ROMID,num);
//   //WWDG_SetCounter(0);
//    onewire_writebyte(READ_SCRATCHPAD);//读取包含配置寄存器在内的所有暂存器内容命令
//
//	for(i=0; i < sizeof(MDC02_SCRATCHPAD_READ); i++)
//    {
//		*scr++ = onewire_read();
//	}
//
//    return TRUE;
//}
//
//
//
//bool mdc02_readc2c3c4_skiprom(uint8_t *scr,int num)
//{
//    int16_t i;
//
////    if(onewire_resetcheck() != 0){
////			return FALSE;
////	}
////
////    read_command();
//	send_matchrom(device_t.ROMID,num);
//
//    onewire_writebyte(READ_C2C3C4);//读取电容通道 2、 3 和 4 命令
//
//	for(i=0; i < sizeof(MDC02_C2C3C4); i++)
//   {
//	    *scr++ = onewire_read();
//	}
//
//    return TRUE;
//}
//////
//////
//////
//bool mdc02_capconfigureoffset(float Coffset,int num)
//{
//	uint8_t CosCfg, Cosbits;
//	float b=Coffset+0.25;
//	CosCfg = captocoscfg(b);
//
//	if(!(CosCfg & ~0x1F)) {
//		Cosbits = COS_RANGE_5BIT;
//	}
//	else if(!(CosCfg & ~0x3F)) {
//		Cosbits = COS_RANGE_6BIT;
//	}
//	else if(!(CosCfg & ~0x7F)){
//		Cosbits = COS_RANGE_7BIT;
//	}
//	else{
//		Cosbits = COS_RANGE_8BIT;
//	}
////	PRINT("%d  %d\r\n",CosCfg,Cosbits);
//	writecosconfig(CosCfg, Cosbits,num);
//
//	return TRUE;
//}
//
///**
//  * @brief  写偏置电容配置寄存器和有效位宽设置
//  * @param  Coffset：偏置配置寄存器的数值
//  * @param  Cosbits：偏置配置寄存器有效位宽，可能为：
//	*		@COS_RANGE_5BIT
//	*		@COS_RANGE_6BIT
//	*		@COS_RANGE_7BIT
//	*		@COS_RANGE_8BIT
//  * @retval 状态
//*/
//bool writecosconfig(uint8_t Coffset, uint8_t Cosbits,int num)
//{
//	uint8_t scrb[sizeof(MDC02_SCRPARAMETERS)];
//	MDC02_SCRPARAMETERS *scr = (MDC02_SCRPARAMETERS *) scrb;
//
//	/*读15个字节。第5字节是偏置电容配置寄存器，第10字节是量程电容配置寄存器，最后字节是前14个的校验和--CRC。*/
//	if(mdc02_readparameters_skiprom(scrb,num) == FALSE)
//	{
//		return FALSE;   /*读寄存器失败*/
//	}
//
//	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/
//  	if(scrb[sizeof(MDC02_SCRPARAMETERS)-1] != onewire_crc8(&scrb[0], sizeof(MDC02_SCRPARAMETERS)-1))
//  	{
//		return FALSE;  /*CRC验证未通过*/
//  	}
//
//	scr->Cos = Coffset;
//	scr->Cfb = (scr->Cfb & ~CFB_COSRANGE_Mask) | Cosbits;
////	PRINT("write cosconfig:%d %f\r\n",scr->Cos,scr->Cfb);
//
//	mdc02_writeparameters_skiprom(scrb,num);
//
//  	return TRUE;
//}
//
///**
//  * @brief  将偏置电容数值（pF）转换为对应的偏置电容配置
//  * @param  osCap：偏置电容的数值
//  * @retval 对应偏置配置寄存器的数值
//*/
//uint8_t captocoscfg(float osCap)
//{
//	int i; 
//	uint8_t CosCfg = 0x00;
//
//	for(i = 7; i >= 0; i--)
//	{
//		if(osCap >= COS_Factor[i])
//		{
//			CosCfg |= (0x01 << i);
//			osCap -= COS_Factor[i];
//		}
//	}
//	return CosCfg;
//}
//
///**
//  * @brief  配置电容测量范围
//  * @param  Cmin：要配置测量范围的低端。
//  * @param  Cmax：要配置测量范围的高端。
//  * @retval 状态
//*/
//bool mdc02_capconfigurerange(float Cmin, float Cmax,int num)
//{ 
//	float Cfs, Cos;
//
////	if(!((Cmax <= 119.0) && (Cmax > Cmin) && (Cmin >= 0.0) && ((Cmax-Cmin) <= 31.0)))
////	return FALSE;	//The input value is out of range.
//
//	Cos = (Cmin + Cmax)/2.0;
//	Cfs = (Cmax - Cmin)/2.0;
//
//	mdc02_capconfigureoffset(Cos,num);
//	mdc02_capconfigurefs(Cfs,num);
//
//	return TRUE;
//}
//
///**
//  * @brief  配置量程电容
//  * @param  Cfs：要配置的量程电容数值。范围+/-（0.281~15.49） pF。
//  * @retval 状态
//*/
//bool mdc02_capconfigurefs(float Cfs,int num)
//{
//	uint8_t Cfbcfg;
//
//	Cfs = (Cfs + 0.1408);
//	Cfbcfg = caprangetocfbcfg(Cfs);
////	PRINT("before write cfbcfg: %d\r\n",Cfbcfg);
//
//	writecfbconfig(Cfbcfg,num);
//
//	return TRUE;
//}
//
///**
//  * @brief  将量程电容数值（pF）转换为对应的量程电容配置
//  * @param  fsCap：量程电容的数值
//  * @retval 对应量程配置的数值
//*/
//uint8_t caprangetocfbcfg(float fsCap)
//{
//	int8_t i; 
//	uint8_t CfbCfg = 0x00;
//
//	fsCap = fsCap * (3.6/0.507);
//
//	fsCap -= CFB.Cfb0;
//
//	for(i = 5; i >= 0; i--)
//	{
//		if(fsCap >= CFB.Factor[i])
//		{
//			fsCap -= CFB.Factor[i];
//			CfbCfg |= (0x01 << i);
//		}
//	}
//
//	return CfbCfg;
//}
///**
//  * @brief  写量程电容配置寄存器
//  * @param  Cfb：量程配置寄存器低6位的内容
//  * @retval 状态
//*/
//bool writecfbconfig(uint8_t Cfb,int num)
//{
//	uint8_t scrb[sizeof(MDC02_SCRPARAMETERS)];
//	MDC02_SCRPARAMETERS *scr = (MDC02_SCRPARAMETERS *) scrb;
//
//	/*读15个字节。第5字节是偏置电容配置寄存器，第10字节是量程电容配置寄存器，最后字节是前14个的校验和--CRC。*/
//	if(mdc02_readparameters_skiprom(scrb,num) == FALSE)
//	{
//		return FALSE;   /*读寄存器失败*/
//	}
//
//	/*计算接收的前14个字节的校验和，并与接收的第15个CRC字节比较。*/
//  	if(scrb[sizeof(MDC02_SCRPARAMETERS)-1] != onewire_crc8(&scrb[0], sizeof(MDC02_SCRPARAMETERS)-1))
//  	{
//  		return FALSE;  /*CRC验证未通过*/
//	}
//
//	scr->Cfb &= ~CFB_CFBSEL_Mask;
//	scr->Cfb |= Cfb;
////	PRINT("write cfb:%d\r\n",scr->Cfb);
//
//	mdc02_writeparameters_skiprom(scrb,num);
//	return TRUE;
//}
//
//
////
////
/////**
////  * @brief  读电容通道2，3和4的测量结果。和 @ConvertCap联合使用
////  * @param  icap：数组指针
////  * @retval 读结果状态
////*/
//bool readcapc2c3c4(uint16_t *iCap,int num)
//{
//	uint8_t scrb[sizeof(MDC02_C2C3C4)];
//	MDC02_C2C3C4 *scr = (MDC02_C2C3C4 *) scrb;
//
//	/*读6个字节。每两个字节依序分别为通道2、3和4的测量结果，最后字节是前两个的校验和--CRC。*/
//	if(mdc02_readc2c3c4_skiprom(scrb,num) == FALSE)
//	{
//		return FALSE;  /*读寄存器失败*/
//	}
//   //WWDG_SetCounter(0);
//	/*计算接收的前两个字节的校验和，并与接收的第3个CRC字节比较。*/
////  if(scrb[3] != onewire_crc8(scrb, 2))
////  {
////		return FALSE;  /*CRC验证未通过*/
////  }
//
//	*iCap= (uint16_t)scr->C2_msb<<8 | scr->C2_lsb;
//	PRINT("%.2x\r\n",*iCap);
////	iCap[1] = (uint16_t)scr->C3_msb<<8 | scr->C3_lsb;
////	iCap[2] = (uint16_t)scr->C4_msb<<8 | scr->C4_lsb;
//	return TRUE;
//}
//
////
///**
//  * @brief  把16位二进制补码表示的温度输出转换为以摄氏度为单位的温度读数
//  * @param  out：有符号的16位二进制温度输出
//  * @retval 以摄氏度为单位的浮点温度
//*/
//float mdc02_outputtotemp(int16_t out)
//{
//	return ((float)out/256.0 + 40.0);
//}
//
//
//
//float mdc02_outputtocap(uint16_t out, float Co, float Cr)
//{
//	return (2.0*(out/65535.0-0.5)*Cr+Co);
//}
//
//
//
////多个字节的crc8算法
//uint8_t onewire_crc8(uint8_t *serial, uint8_t length)
//{
//    uint8_t crc8 = 0x00;
//    uint8_t pDataBuf;
//    uint8_t i;
//
//    while(length--) {
////        pDataBuf = *serial++;
//		crc8 ^= *serial++;
//        for(i=0; i<8; i++) {
//            if((crc8^(pDataBuf))&0x01) {
//                crc8 ^= 0x18;
//                crc8 >>= 1;
//                crc8 |= 0x80;
//            }
//            else {
//                crc8 >>= 1;
//            }
//            pDataBuf >>= 1;
//    }
//  }
//    return crc8;
//}
//
//void onewire_find_alldevice(){
////	device device_t;
//	uint8_t device_num;
//	
//	device_num=Search_ROM(device_t.ROMID);
//	PRINT("总线上设备个数为：%d\r\n",device_num);
//	for(int j=0; j<device_num;j++){
//		PRINT(" ROMID[%d]: ",j);
//
//		for(int i=0; i<8;i++){
//			PRINT(" %2X", device_t.ROMID[j][i]);
//		}
//		PRINT("\r\n");
//	}
//}
//
//bool send_matchrom(uint8_t (*FoundROM)[8],int num)
//{
//   //WWDG_SetCounter(0);
//	if(onewire_resetcheck() != 0){ 			//检查总线上从机是否存在
//		return FALSE;
//	}
////	num=Search_ROM(device_t.ROMID);
//	onewire_writebyte(MATCH_ROM);// 匹配 ROM 指令
//	for(int i = 0;i < 8;i++){
////		PRINT(" %2X", FoundROM[num][i]);
//		onewire_writebyte(FoundROM[num][i]);
//	}
////    WWDG_SetCounter(0);
//	return TRUE;
//}
//float my18b20_output_to_temp(int16_t out,int num)
//{
//	return ((float)out/16.0);	
//}
//
//
//void romid_crc(int num){
//	if(device_t.ROMID[num][7] == onewire_crc8(&device_t.ROMID[num][0],7)){
//	    device_t.type = MY18B20Z ;
//		PRINT("\r\n");
//		PRINT("this temp device serial number is:%d\r\n",num);
//		PRINT("this device is:%d\r\n",  device_t.type);
//	}
//	else{
//		if(device_t.ROMID[num][7] == 0 && (device_t.ROMID[num][6]) == 0){
//		    device_t.type = MDC02;
////		    strcpy(&device_t.type,"mdc02");
//			PRINT("\r\n");
//			PRINT("this mdc02 device serial number is:%d\r\n",num);
//			PRINT("this device is:%d\r\n", device_t.type);
//		}
//	}
//}
//
////////////////******************读取温度*************///////////////////
//int read_temp(int num){ 
//	float fTemp=0; 
//	uint16_t iTemp=0; 
//	
//	if(converttemp(num) == TRUE)
//	{
//    	DelayUs(15);
//		read_tempwaiting(&iTemp,num);
//		fTemp=my18b20_output_to_temp((int16_t)iTemp,num);
//		PRINT("Array_index= %d ,T= %3.3f \r\n",num, fTemp);
//		PRINT("\r\n");
//		modbus_ireg_update(MB_IREG_ADDR_MY18B20Z,fTemp);
//	}
//	else
//	{
//		PRINT("\r\n No MDC02");
//	}
////    WWDG_SetCounter(0);
//	DelayUs(990);
////    WWDG_SetCounter(0);
//				
//	return 1;
//}
//
//static int judge_status(float a,float b){
//	if(a > 5.0 && b > 5.0){
//		PRINT("进水了\r\n");
//		return 1;
//	}
//	else{
//		return 0;
//	}
//}
//static int read_caps(int num)
//{	
//	float fcap1, fcap2, fcap3, fcap4; uint16_t iTemp, icap1, icap[1];
//	uint8_t status, cfg;
//	uint8_t status_wids=0;
//	
//	setcapchannel(CAP_CH1CH2_SEL,num);
//   //WWDG_SetCounter(0);
//	readstatusconfig((uint8_t *)&status, (uint8_t *)&cfg,num);
//   //WWDG_SetCounter(0);
//	
//	if(convertcap(num) == FALSE)
//	{
//		PRINT("No MDC02\r\n");
//	}
//	else{
//		WWDG_SetCounter(0 );
//		DelayUs(15);
//		readcapconfigure(&onewire_dev.CapCfg_offset, &onewire_dev.CapCfg_range,num);
////		PRINT("%f  %.5f\r\n",onewire_dev.CapCfg_offset,onewire_dev.CapCfg_range);
//		readstatusconfig((uint8_t *)&status, (uint8_t *)&cfg,num);
//
//		readtempcap1(&iTemp, &icap1,num);
//		if(readcapc2c3c4(&icap[0],num) == FALSE){
//			PRINT("read cap2 error");
//		}
//		fcap1 = mdc02_outputtocap(icap1, onewire_dev.CapCfg_offset, onewire_dev.CapCfg_range);
//		fcap2 = mdc02_outputtocap(icap[0], onewire_dev.CapCfg_offset, onewire_dev.CapCfg_range);
//		PRINT("Array_index= %d, C1=%4d , %6.3f pf  ,C2=%4d, %6.3f pf , S=%02X   C=%02X\r\n",num, icap1, fcap1, icap[0], fcap2, status, cfg);
//
//		status_wids=judge_status(fcap1,fcap2);
//		PRINT("\r\n");
//		OLED_ShowNum(82, 16, status_wids, 1, 16, 1);
//
//		OLED_Refresh(0);
//		modbus_di_update(MB_DI_ADDR_WIDS,status_wids);
//		
//		}
//
//	DelayUs(990);
//	return 1;		
//}
//////
//static int read_tempc1(int num)
//{
//	uint16_t iTemp, iCap1; 
//	float fTemp, fCap1;
//		
//	readcapconfigure(&onewire_dev.CapCfg_offset, &onewire_dev.CapCfg_range,num);
//	setcapchannel(CAP_CH1_SEL,num);
//
//	if(convert_tempcap1(num) == TRUE)
//	{
//		DelayUs(15);		
//		if(readtempcap1(&iTemp, &iCap1,num) == TRUE) 
//		{	
////			PRINT("%2x\r\n",iTemp);
//			fTemp=mdc02_outputtotemp(iTemp);
//			fCap1=mdc02_outputtocap(iCap1, onewire_dev.CapCfg_offset, onewire_dev.CapCfg_range);
//           //WWDG_SetCounter(0);
//			PRINT(" Array_index=%d ,T= %3.3f ℃ C1= %6.3f pF\r\n",num, fTemp, fCap1);
//		}
//	}
//	else
//	{
//		PRINT("\r\n No MDC02");
//	}
////    WWDG_SetCounter(0);
//	DelayUs(990);
////    WWDG_SetCounter(0);
//	return 1;		
//}
//////  * @brief  等待转换结束后读测量结果。和@ConvertTemp联合使用
//////  * @param  iTemp：返回的16位温度测量结果
//////  * @retval 读状态
//////*/
//bool read_tempwaiting(uint16_t *iTemp,int num)
//{
//	uint8_t scrb[sizeof(MDC02_SCRATCHPAD_READ)];
//	MDC02_SCRATCHPAD_READ *scr = (MDC02_SCRATCHPAD_READ *) scrb;
//
//	/*读9个字节。前两个是温度转换结果，最后字节是前8个的校验和--CRC。*/
//	if(mdc02_readscratchpad_skiprom(scrb,num) == FALSE)
//	{
//		return FALSE;  /*读寄存器失败*/
//	}
//
//	/*计算接收的前8个字节的校验和，并与接收的第9个CRC字节比较。*/
//    if(scrb[8] != onewire_crc8(&scrb[0], 8))
//    {
//    	return FALSE;  /*CRC验证未通过*/
//    }
//
//	/*将温度测量结果的两个字节合成为16位字。*/
//    *iTemp=(uint16_t)scr->T_msb<<8 | scr->T_lsb;
//
//    return TRUE;
//}
//
//
//
//
//void dev_type_choose_function(){
//	uint16_t dev_num;
//	volatile int num_i;
//	dev_num=Search_ROM(device_t.ROMID);
//	for(num_i = 0;num_i < dev_num;num_i++){
//		romid_crc(num_i);
//		read_sampling_mode(MDC02_REPEATABILITY_HIGH,MDC02_MPS_1Hz,num_i);
//		if(device_t.type == MDC02){
////           //WWDG_SetCounter(0);
//			mdc02_range(0, 30, num_i);
//			read_tempc1(num_i);
//            //WWDG_SetCounter(0);   
//			read_caps(num_i);
////            WWDG_SetCounter(0);
//		}
//		else{
////           //WWDG_SetCounter(0);
//			read_temp(num_i);
////           //WWDG_SetCounter(0);
//		}
//	}
//}
//
//
//
//
//
///*
//  * @brief  设置偏置电容offset
//*/
////int mdc02_offset(float Co)
////{
////	PRINT("Co= %5.2f\r\n", Co);
////	if(!((Co >=0.0) && (Co <= 103.5))) {
////		PRINT(" %s", "The input is out of range");
////		return 0;
////	}
////
////	mdc02_capconfigureoffset(Co,num);
////	return 1;
////}
///*
//  * @brief  设置电容测量范围
//  * 请勿设置超出电容量程0~119pf,请勿超出最大range:±15.5pf
//*/
//int mdc02_range(float Cmin,float Cmax,int num)
//{ 
////		printf("\r\nCmin= %3.2f Cmax=%3.2f", Cmin, Cmax);
//	if(!((Cmax <= 119.0) && (Cmax > Cmin) && (Cmin >= 0.0) && 
//			((Cmax-Cmin) <= 31.0)))  
//	{
//		PRINT(" %s", "The input is out of range"); 
//		return 0;
//	}
//		
//	mdc02_capconfigurerange(Cmin, Cmax,num);
//		
//	readcapconfigure(&onewire_dev.CapCfg_offset, &onewire_dev.CapCfg_range,num);
////	PRINT("%.2f  %.5f\r\n",CapCfg_offset,CapCfg_range);
//		
//	return 1;
//}
//
///*
//  * @brief  设置配置寄存器
//  * MPS:  000   001     010    011    100   101
//  *      单次  0.5次/S 1次/S  2次/S  4次/S 10次/S
//  * Repeatability:  00: 低重复性
//  *                 01：中重复性
//  *                 10：高重复性
//*/
//int read_sampling_mode(int repeatability,int mps,int num)
//{ 
//	int status, cfg;
//	setconfig(mps & 0x07, repeatability & 0x03,num);
//	readstatusconfig((uint8_t *)&status, (uint8_t *)&cfg,num);
////	PRINT("S=%02x C=%02x", status, cfg);
//	
//	return 0;
//}
//
////int mdc02_channel(uint8_t channel)
////{
////	setcapchannel(channel,num);
////									
////	return 1;
////}



