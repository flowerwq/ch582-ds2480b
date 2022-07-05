#ifndef SRC_INCLUDE_UART_DS2480_H_
#define SRC_INCLUDE_UART_DS2480_H_

#include "CH58x_common.h"
// UART connectivity functions (Win32 implementation)
void FlushCOM(void);
int  WriteCOM(int outlen, unsigned char *outbuf);
int  ReadCOM(int inlen, unsigned char *inbuf);
void BreakCOM(void);
void SetBaudCOM(unsigned char new_baud);
void msDelay(int len);
int  OpenCOM(char *port_zstr);
void CloseCOM(void);
int uart_init();
int uart_send_break();
int change_baud_rate(int i);
int uart_reads(uint8_t * j,int k);
int uart_write(uint8_t    *i,int len);






#endif /* SRC_INCLUDE_UART_DS2480_H_ */
