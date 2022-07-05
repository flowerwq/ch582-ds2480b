#ifndef SRC_INCLUDE_ONEWIRE_H_
#define SRC_INCLUDE_ONEWIRE_H_

#include "CH58x_common.h"
// Basic 1-Wire functions
int  OWReset();
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




#endif /* SRC_INCLUDE_ONEWIRE_H_ */
