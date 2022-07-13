#ifndef SRC_INCLUDE_ONEWIRE_H_
#define SRC_INCLUDE_ONEWIRE_H_
#include <stdbool.h>
#include "CH58x_common.h"
// Basic 1-Wire functions
bool  OWReset();
unsigned char OWTouchBit(unsigned char sendbit);
unsigned char OWTouchByte(unsigned char sendbyte);
void OWWriteByte(unsigned char byte_value);
void OWWriteBit(unsigned char bit_value);
unsigned char OWReadBit();
unsigned char OWReadByte();
int OWBlock(unsigned char *tran_buf, int tran_len);
int  OWSearch();
int  OWFirst();
int  OWNext();
int  OWVerify();
void OWTargetSetup(unsigned char family_code);
void OWFamilySkipSetup();

// Extended 1-Wire functions
int OWSpeed(int new_speed);
int OWLevel(int level);
int OWProgramPulse(void);
int OWWriteBytePower(int sendbyte);
int OWReadBitPower(int applyPowerResponse);

unsigned char docrc8(unsigned char value);

// DS2480B utility functions
int DS2480B_Detect(void);
int DS2480B_ChangeBaud(unsigned char newbaud);


int onewire_search();
int find_device();
int find_device_config();

typedef struct{
    int LastDiscrepancy;
    int LastFamilyDiscrepancy;
    int LastDeviceFlag;
    unsigned char crc8;
    unsigned char ROM_NO[8];
}search_device;

typedef struct{
    // DS2480B state
    int ULevel; // 1-Wire level
    int UBaud;  // baud rate
    int UMode;  // command or data mode state
    int USpeed; // 1-Wire communication speed
    int ALARM_RESET_COMPLIANCE; // flag for DS1994/DS2404 'special' reset
}ds2480_state;

struct dev_slave{
    device_type_t type;
    uint8_t ROMID[MAXNUM_SWITCH_DEVICE_OUT][8];
};


struct dev_slave device_t;



#endif /* SRC_INCLUDE_ONEWIRE_H_ */
