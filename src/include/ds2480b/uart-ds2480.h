#ifndef SRC_INCLUDE_UART_DS2480_H_
#define SRC_INCLUDE_UART_DS2480_H_

#include "CH58x_common.h"


typedef struct{
//	uint8_t buffer;
//	
//	int LastDiscrepancy;
//	int LastFamilyDiscrepancy;
//	int LastDeviceFlag;
//	unsigned char crc8;
	/****全局变量：保存和电容配置寄存器对应的偏置电容和量程电容数值****/
	float CapCfg_offset, CapCfg_range;
	uint8_t CapCfg_ChanMap, CapCfg_Chan;
//	unsigned char ROM_NO[8];
}onewire;


typedef enum
{
	//ROM command
    SKIP_ROM            	= 0xcc,
    READ_ROM            	= 0x33,
    MATCH_ROM           	= 0x55,
	SEARCH_ROM           	= 0xf0, 
	ALARM_SEARCH			= 0xec,
	//Function command
    CONVERT_C           	= 0x66,
    CONVERT_T           	= 0x44,
	CONVERT_TC1             = 0x10,	
	READ_SCRATCHPAD     	= 0xbe,
	WRITE_SCRATCHPAD     	= 0x4e,
	READ_TC1             	= 0xcf,	
	READ_C2C3C4			    = 0xdc,
	READ_PARAMETERS      	= 0x8b,
	WRITE_PARAMETERS     	= 0xab,	
	COPY_PAGE0				= 0x48,
	READ_SCRATCHPAD_EXT  	= 0xdd,
	WRITE_SCRATCHPAD_EXT 	= 0x77,
} MDC02_OW_CMD;


typedef struct
{
	uint8_t Res[3];
	uint8_t Ch_Sel;					/*电容通道选择寄存器，RW*/
	uint8_t Cos;						/*偏置电容配置寄存器，RW*/
	uint8_t Res1;				
	uint8_t T_coeff[3];			
	uint8_t Cfb;						/*量程电容配置寄存器，RW*/
	uint8_t Res2;
	uint8_t Res3[2];
	uint8_t dummy8;
	uint8_t crc_para;				/*CRC for byte0-13, RO*/
} MDC02_SCRPARAMETERS;

typedef struct
{
	uint8_t T_lsb;					/*The LSB of 温度结果, RO*/
	uint8_t T_msb;					/*The MSB of 温度结果, RO*/
	uint8_t C1_lsb;					/*The LSB of 电容通道C1, RO*/
	uint8_t C1_msb;					/*The MSB of 电容通道C1, Ro*/
	uint8_t Tha_set_lsb;		
	uint8_t Tla_set_lsb;		
	uint8_t Cfg;						/*系统配置寄存器, RW*/
	uint8_t Status;					/*系统状态寄存器, RO*/
	uint8_t crc_scr;				/*CRC for byte0-7, RO*/
} MDC02_SCRATCHPAD_READ;
typedef struct
{	
	int8_t Tha_set_lsb;				
	int8_t Tla_set_lsb;			
	uint8_t Cfg;						/*系统配置寄存器, RW*/
} MDC02_SCRATCHPAD_WRITE;

typedef struct
{
	uint8_t tha_clear;				
	uint8_t tla_clear;					
	uint8_t hha_set;					
	uint8_t hla_set;					
	uint8_t hha_clear;				
	uint8_t hla_clear;					
	uint8_t udf[5];							
	uint8_t MPW_test;					
	uint8_t crc_ext;						
} MDC02_SCRATCHPADEXT;

typedef struct
{
	uint8_t Family;				/*Family byte, RO*/
	uint8_t Id[6];				/*Unique ID, RO*/
	uint8_t crc_rc;				/*Crc code for byte0-7, RO*/
} MDC02_ROMCODE;

typedef enum{
CAP_CH1_SEL =0x01,
CAP_CH2_SEL =0x02,
CAP_CH3_SEL =0x03,
CAP_CH4_SEL =0x04, 
CAP_CH1CH2_SEL =0x05 ,
CAP_CH1CH2CH3_SEL = 0x06,
CAP_CH1CH2CH3CH4_SEL =0x07,
}onewire_capch;
typedef struct
{	
	uint8_t C2_lsb;				/*The LSB of C2, RO*/
	uint8_t C2_msb;				/*The MSB of C2, RO*/
	uint8_t C3_lsb;				/*The LSB of C3, RO*/
	uint8_t C3_msb;				/*The MSB of C3, RO*/
	uint8_t C4_lsb;				/*The LSB of C4, RO*/
	uint8_t C4_msb;				/*The MSB of C4, RO*/
/*crc*/	
} MDC02_C2C3C4;


/*Bit definition of CFB register*/
typedef enum{
	CFB_COSRANGE_Mask =	0xC0,
	CFB_CFBSEL_Mask   =	0x3F,	
	CFB_COS_BITRANGE_5 = 0x1F,
 	CFB_COS_BITRANGE_6 = 0x3F,
 	CFB_COS_BITRANGE_7 = 0x7F,
 	CFB_COS_BITRANGE_8 = 0xFF,
	
 	COS_RANGE_5BIT     = 0x00,
 	COS_RANGE_6BIT	   = 0x40,
 	COS_RANGE_7BIT	   = 0x80,
 	COS_RANGE_8BIT     = 0xC0, 
}cfb_config;
typedef enum device_type{
	MDC02=1,
	MY18B20Z,
}device_type_t;



// UART connectivity functions (Win32 implementation)

//int uart_send_break();
int change_baud_rate(int i);
int uart_reads(uint8_t * j,int k);
int uart_write(uint8_t    *i,int len);

#include <stdbool.h>
/******************  Bit definition for feeadback capacitor register  *******/
#define MDC02_CFEED_OSR_MASK           	   	0xC0
#define MDC02_CFEED_CFB_MASK           	   	0x3F
/*Bit definition of Ch_Sel register*/
#define CCS_CHANNEL_MASK   					0x07
#define MAXNUM_SWITCH_DEVICE_OUT  100

#define CFG_CLKSTRETCH_Mask   	0x20
#define CFG_MPS_Mask   		  	0x1C
#define CFG_Repeatbility_Mask 	0x03
#define CFG_MPS_Single  	0x00
#define CFG_MPS_Half  		0x04
#define CFG_MPS_1  			0x08
#define CFG_MPS_2 			0x0C
#define CFG_MPS_4 			0x10
#define CFG_MPS_10 			0x14
#define CFG_Repeatbility_Low   	0x00
#define CFG_Repeatbility_Medium 0x01
#define CFG_Repeatbility_High 	0x02
#define CFG_ClkStreatch_Disable (0x00 << 5)
#define CFG_ClkStreatch_Enable 	(0x01 << 5)

#define MDC02_REPEATABILITY_LOW               (0x00 << 0)
#define MDC02_REPEATABILITY_MEDIUM            (0x01 << 0)
#define MDC02_REPEATABILITY_HIGH              (0x02 << 0)
/******************  Bit definition for TTrim in parameters  *******/
#define MDC02_MPS_SINGLE					            (0x00 << 2)
#define MDC02_MPS_0P5Hz					            	(0x01 << 2)
#define MDC02_MPS_1Hz					            		(0x02 << 2)
#define MDC02_MPS_2Hz					            		(0x03 << 2)
#define MDC02_MPS_4Hz					            		(0x04 << 2)
#define MDC02_MPS_10Hz					            	(0x05 << 2)

#define MDC02_CLKSTRETCH_EN					          (0x01 << 5)
/******************  Bit definition for status register  *******/
#define MDC02_STATUS_CONVERTMODE_MASK          0x81
#define MDC02_STATUS_I2CDATACRC_MASK           0x20
#define MDC02_STATUS_I2CCMDCRC_MASK            0x10
#define MDC02_STATUS_SYSRESETFLAG_MASK         0x08



void *memset(void *s, int ch, size_t n);


int read_tempcap1(void);


/////////////****  search rom_id **** ////////////
void read_function(void);
uint8_t onewire_crc8(uint8_t *serial, uint8_t length);
int onewire_search();
int onewire_next();
int onewire_first();
int onewire_read_rom(void);
void onewire_targetsetup(unsigned char family_code);
void onewire_familyshipsetup();
void onewire_finddevic_fun();

void onewire_find_alldevice();

int onewire_resetcheck();
int mdc02_channel(uint8_t channel);


uint8_t captocoscfg(float osCap);
float cfbcfgtocaprange(uint8_t fbCfg);
float coscfgtocapoffset(uint8_t osCfg);
float mdc02_outputtocap(uint16_t out, float Co, float Cr);
float mdc02_outputtotemp(int16_t out);

////void getcfg_capoffset(float *Coffset);
////void getcfg_caprange(float *Crange);
////bool setcapchannel(uint8_t channel);
////*** write ***////

bool mdc02_writescratchpadext_skiprom(uint8_t *scr);
////bool writecosconfig(uint8_t Coffset, uint8_t Cosbits);
////bool mdc02_writeparameters_skiprom(uint8_t *scr);
////bool mdc02_capconfigurerange(float Cmin, float Cmax);
////bool writecfbconfig(uint8_t Cfb);


/////////////**** read ****/////////////
////bool read_tempwaiting(uint16_t *iTemp);

int read_cap(void);
////bool readcfbconfig(uint8_t *Cfb);
////bool mdc02_readparameters_skiprom(uint8_t *scr);
////bool readcosconfig(uint8_t *Coscfg);
////bool readcapconfigure(float *Coffset, float *Crange);
////bool readstatusconfig(uint8_t *status, uint8_t *cfg);
////bool mdc02_capconfigureoffset(float Coffset);
////bool readtempcap1(uint16_t *iTemp, uint16_t *iCap1);
////bool mdc02_readc2c3c4_skiprom(uint8_t *scr);
////bool readcapc2c3c4(uint16_t *iCap);
////bool convertcap(void);
////bool mdc02_readscratchpad_skiprom(uint8_t *scr);
int mdc02_offset(float Co);
////int mdc02_range(float Cmin,float Cmax);
////bool mdc02_capconfigurefs(float Cfs);
uint8_t caprangetocfbcfg(float fsCap);


//////////////////////**********************////////////////////////

//bool send_matchrom();
//int read_tmp();
//int read_caps();
//bool setcapchannel(uint8_t channel);
//bool readcapconfigure(float *Coffset, float *Crange);
//bool mdc02_writeparameters_skiprom(uint8_t *scr);
//bool mdc02_readparameters_skiprom(uint8_t *scr);
//bool readstatusconfig(uint8_t *status, uint8_t *cfg);
//bool readcapconfigure(float *Coffset, float *Crange);
//void getcfg_caprange(float *Crange);
//bool readcfbconfig(uint8_t *Cfb);
//bool readcosconfig(uint8_t *Coscfg);
//bool readtempcap1(uint16_t *iTemp, uint16_t *iCap1);
//bool readcapc2c3c4(uint16_t *iCap);
//bool mdc02_readc2c3c4_skiprom(uint8_t *scr);
//bool mdc02_capconfigurerange(float Cmin, float Cmax);
//int mdc02_range(float Cmin,float Cmax);
//bool mdc02_capconfigurerange(float Cmin, float Cmax);
//bool mdc02_capconfigureoffset(float Coffset);
//bool mdc02_capconfigurefs(float Cfs);
//bool writecfbconfig(uint8_t Cfb);
//bool writecosconfig(uint8_t Coffset, uint8_t Cosbits);


//bool convertcap();



bool send_matchrom(uint8_t (*FoundROM)[8],int num);

bool mdc02_readparameters_skiprom(uint8_t *scr,int num);
bool  mdc02_readscratchpad_skiprom(uint8_t *scr,int num);
bool mdc02_readc2c3c4_skiprom(uint8_t *scr,int num);
bool readstatusconfig(uint8_t *status, uint8_t *cfg,int num);
bool readcfbconfig(uint8_t *Cfb,int num);
bool readcapconfigure(float *Coffset, float *Crange,int num);
bool readtempcap1(uint16_t *iTemp, uint16_t *iCap1,int num);
bool readcapc2c3c4(uint16_t *iCap,int num);
bool read_tempwaiting(uint16_t *iTemp,int num);
bool readcosconfig(uint8_t *Coscfg,int num);
int read_temp(int num);

void getcfg_caprange(float *Crange,int num);
void getcfg_capoffset(float *Coffset,int num);
bool setcapchannel(uint8_t channel,int num);
bool convert_tempcap1(int num);
bool convertcap(int num);
void dev_type_choose_function();

bool mdc02_writeparameters_skiprom(uint8_t *scr, int num);
bool writecosconfig(uint8_t Coffset, uint8_t Cosbits,int num);
bool writecfbconfig(uint8_t Cfb,int num);
bool mdc02_capconfigureoffset(float Coffset,int num);
bool mdc02_capconfigurefs(float Cfs,int num);
bool mdc02_capconfigurerange(float Cmin, float Cmax,int num);

int mdc02_range(float Cmin,float Cmax,int num);
void romid_crc(int num);

bool converttemp(int num);
float my18b20_output_to_temp(int16_t out,int num);
bool setconfig(uint8_t mps, uint8_t repeatability,int num);
bool mdc02_writescratchpad_skiprom(uint8_t *scr,int num);
int read_sampling_mode(int repeatability,int mps,int num);

int read_tmp(int num);




#endif /* SRC_INCLUDE_UART_DS2480_H_ */
