#include "ds2480b/commun.h"
#include "ds2480b/uart-ds2480.h"
#include "ds2480b/common.h"
#include "ds2480b/onewire.h"


/*
// debug mode
int dodebug=FALSE;

// Win32 serial globals
HANDLE ComID;
OVERLAPPED osRead,osWrite;


*/


//int uart_init(){
//    GPIOPinRemap(ENABLE, RB_PIN_UART0);
//    GPIOA_SetBits(GPIO_Pin_14);
//    GPIOA_ModeCfg(GPIO_Pin_15,GPIO_ModeIN_PU);
//    GPIOA_ModeCfg(GPIO_Pin_14,GPIO_ModeOut_PP_20mA);
//    UART0_DefInit();
//    UART0_BaudRateCfg(PARMSET_9600);//
//
//    UART0_ByteTrigCfg(UART_1BYTE_TRIG);
//	UART0_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
//	PFIC_EnableIRQ(UART0_IRQn);
//
//}

int uart_send_break(){
//    GPIOA_ResetBits(GPIO_Pin_14);
//    DelayMs(2);
//    GPIOA_SetBits(GPIO_Pin_14);

    R8_UART0_LCR |= RB_LCR_BREAK_EN;
    DelayMs(2);
//    R8_UART0_LCR &= 0xbf;
    R8_UART0_LCR &= ~(1 << 6);
//    PRINT("LCR:%02x\r\n ",R8_UART0_LCR);
             
}

int uart_write(uint8_t *i,int len){
    UART0_SendString(i, len);
    while(R8_UART0_TFC || !(R8_UART0_LSR & RB_LSR_TX_ALL_EMP));
   
    return 1;
}

int uart_reads(uint8_t *j,int k){
    uint8_t len=0;
    for(int i = 0;i < k;i++){
        *j++ = UART0_RecvByte();
        len++;
    }
   
    return len;

}


int change_baud_rate(int i){
     UART0_BaudRateCfg(i);
     return 1;
    
}

//__INTERRUPT
//__HIGH_CODE
//void UART0_IRQHandler(void)
//{
//    volatile uint8_t i;
//    uint8_t RxBuff[100];
//
//    switch(UART0_GetITFlag())
//    {
//        case UART_II_LINE_STAT: // 线路状态错误
//        {
//            UART0_GetLinSTA();
//            break;
//        }
//
//        case UART_II_RECV_RDY: // 数据达到设置触发点
//            while(R8_UART0_RFC){
//                RxBuff[i++]=UART0_RecvByte();
//            }
//            break;
//
//        case UART_II_RECV_TOUT: // 接收超时，暂时一帧数据接收完成
//            while(R8_UART0_RFC){
//                RxBuff[i++]=UART0_RecvByte();
//            }
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
//


//
////---------------------------------------------------------------------------
////-------- WIN32 COM functions
////---------------------------------------------------------------------------
//
////---------------------------------------------------------------------------
//// Attempt to open a com port.  Keep the handle in ComID.
//// Set the starting baud rate to 9600.
////
//// 'port_zstr' - zero terminate port name.  For this platform
////               use format COMX where X is the port number.
////
////
//// Returns: TRUE(1)  - success, COM port opened
////          FALSE(0) - failure, could not open specified port
////
//
//
//int OpenCOM(char *port_zstr)
//{
//   char tempstr[80];
//   short fRetVal;
//   COMMTIMEOUTS CommTimeOuts;
//   DCB dcb;//结构体
//
//   // open COMM device
//   if ((ComID =CreateFile( port_zstr, GENERIC_READ | GENERIC_WRITE,
//                  0,
//                  NULL,                 // no security attrs
//                  OPEN_EXISTING,
//                  FILE_FLAG_OVERLAPPED, // overlapped I/O
//                  NULL )) == (HANDLE) -1 )
//   {
//      ComID = 0;
//      return FALSE;
//   }
//   else
//   {
//      // create events for detection of reading and write to com port
//      sprintf(tempstr,"COMM_READ_OVERLAPPED_EVENT_FOR_%s",port_zstr);
//      osRead.hEvent = CreateEvent(NULL,TRUE,FALSE,tempstr);
//      sprintf(tempstr,"COMM_WRITE_OVERLAPPED_EVENT_FOR_%s",port_zstr);
//      osWrite.hEvent = CreateEvent(NULL,TRUE,FALSE,tempstr);
//
//      // get any early notifications，设置事件掩模来监视指定通信端口上的事件，
//      SetCommMask(ComID, EV_RXCHAR | EV_TXEMPTY | EV_ERR | EV_BREAK);
//
//      // setup device buffers，设置输入输出缓冲区大小
//      SetupComm(ComID, 2048, 2048);
//
//      // purge any information in the buffer，清空缓冲区，
//      //PURGE_TXABORT 终止所有正在进行的 字符输出操作,完成一个正处于等待状态的重叠 i/o操作,他将产生一个事件,指明完成了写操作
//      //PURGE_RXABORT 终止所有正在进行的字符输入操作,完成一个正在进行中的重叠i/o操作,并带有已设置得适当事件
//      //PURGE_TXCLEAR 这个命令指导设备驱动程序清除输出缓冲区，经常与PURGE_TXABORT 命令标志一起使用
//      //PURGE_RXCLEAR 这个命令用于设备驱动程序清除输入缓冲区，经常与PURGE_RXABORT 命令标志一起使用
//
//      PurgeComm(ComID, PURGE_TXABORT | PURGE_RXABORT |
//                           PURGE_TXCLEAR | PURGE_RXCLEAR );
//
//      // set up for overlapped non-blocking I/O
//      CommTimeOuts.ReadIntervalTimeout = 0;
//      CommTimeOuts.ReadTotalTimeoutMultiplier = 20;
//      CommTimeOuts.ReadTotalTimeoutConstant = 40;
//      CommTimeOuts.WriteTotalTimeoutMultiplier = 20;
//      CommTimeOuts.WriteTotalTimeoutConstant = 40;
//      SetCommTimeouts(ComID, &CommTimeOuts);
//
//      // setup the com port，读取串口设置(波特率,校验,停止位,数据位等)
//      GetCommState(ComID, &dcb);
//
//      dcb.BaudRate = CBR_9600;               // current baud rate
//      dcb.fBinary = TRUE;                    // binary mode, no EOF check
//      dcb.fParity = FALSE;                   // enable parity checking
//      dcb.fOutxCtsFlow = FALSE;              // CTS output flow control
//      dcb.fOutxDsrFlow = FALSE;              // DSR output flow control
//      dcb.fDtrControl = DTR_CONTROL_ENABLE;  // DTR flow control type
//      dcb.fDsrSensitivity = FALSE;           // DSR sensitivity
//      dcb.fTXContinueOnXoff = TRUE;          // XOFF continues Tx
//      dcb.fOutX = FALSE;                     // XON/XOFF out flow control
//      dcb.fInX = FALSE;                      // XON/XOFF in flow control
//      dcb.fErrorChar = FALSE;                // enable error replacement
//      dcb.fNull = FALSE;                     // enable null stripping
//      dcb.fRtsControl = RTS_CONTROL_ENABLE;  // RTS flow control
//      dcb.fAbortOnError = FALSE;             // abort reads/writes on error
//      dcb.XonLim = 0;                        // transmit XON threshold
//      dcb.XoffLim = 0;                       // transmit XOFF threshold
//      dcb.ByteSize = 8;                      // number of bits/byte, 4-8
//      dcb.Parity = NOPARITY;                 // 0-4=no,odd,even,mark,space
//      dcb.StopBits = ONESTOPBIT;             // 0,1,2 = 1, 1.5, 2
//      dcb.XonChar = 0;                       // Tx and Rx XON character
//      dcb.XoffChar = 1;                      // Tx and Rx XOFF character
//      dcb.ErrorChar = 0;                     // error replacement character
//      dcb.EofChar = 0;                       // end of input character
//      dcb.EvtChar = 0;                       // received event character
//      
//    //函数设置COM口的设备控制块，成功返回非0，不成功返回0
//      fRetVal = SetCommState(ComID, &dcb);
//   }
//
//   // check if successfull
//   if (!fRetVal)
//   {
//      CloseHandle(ComID);
//      CloseHandle(osRead.hEvent);
//      CloseHandle(osWrite.hEvent);
//      ComID = 0;
//   }
//
//   return fRetVal;
//}
//
////---------------------------------------------------------------------------
//// Closes the connection to the port.
////
//void CloseCOM(void)
//{
//   // disable event notification and wait for thread
//   // to halt
//   SetCommMask(ComID, 0);
//
//   // drop DTR，指导通信设备执行某项扩展功能
//   EscapeCommFunction(ComID, CLRDTR);
//
//   // purge any outstanding reads/writes and close device handle
//   PurgeComm(ComID, PURGE_TXABORT | PURGE_RXABORT |
//                    PURGE_TXCLEAR | PURGE_RXCLEAR );
//   CloseHandle(ComID);
//   CloseHandle(osRead.hEvent);
//   CloseHandle(osWrite.hEvent);
//   ComID = 0;
//}
//
////---------------------------------------------------------------------------
//// Flush the rx and tx buffers
////
//void FlushCOM(void)
//{
//   // purge any information in the buffer
//   PurgeComm(ComID, PURGE_TXABORT | PURGE_RXABORT |
//                    PURGE_TXCLEAR | PURGE_RXCLEAR );
//
//   // debug
//   if (dodebug)
//      printf("__Flush__\n");
//}
//
////--------------------------------------------------------------------------
//// Write an array of bytes to the COM port, verify that it was
//// sent out.  Assume that baud rate has been set.
////
//// 'outlen'   - number of bytes to write to COM port
//// 'outbuf'   - pointer ot an array of bytes to write
////
//// Returns:  TRUE(1)  - success
////           FALSE(0) - failure
////
//int WriteCOM(int outlen, unsigned char *outbuf)
//{
//   BOOL fWriteStat;
//   DWORD dwBytesWritten=0;
//   DWORD ler=0,to;
//   int i;
//
//   // debug
//   if (dodebug)
//   {
//      for (i = 0; i < outlen; i++)
//         printf(">%02X",outbuf[i]);
//      printf("\n");
//   }
//
//   // calculate a timeout
//   to = 20 * outlen + 60;
//
//   // reset the write event
//   ResetEvent(osWrite.hEvent);
//
//   // write the byte
//   fWriteStat = WriteFile(ComID, (LPSTR) &outbuf[0],
//                outlen, &dwBytesWritten, &osWrite );
//
//   // check for an error
//   if (!fWriteStat)
//      ler = GetLastError();
//
//   // if not done writting then wait
//   if (!fWriteStat && ler == ERROR_IO_PENDING)
//   {
//      WaitForSingleObject(osWrite.hEvent,to);
//
//      // verify all is written correctly
//      fWriteStat = GetOverlappedResult(ComID, &osWrite,
//                   &dwBytesWritten, FALSE);
//
//   }
//
//   // check results of write
//   if (!fWriteStat || (dwBytesWritten != (DWORD)outlen))
//      return 0;
//   else
//      return 1;
//}
//
////--------------------------------------------------------------------------
//// Read an array of bytes to the COM port, verify that it was
//// sent out.  Assume that baud rate has been set.
////
//// 'inlen'     - number of bytes to read from COM port
//// 'inbuf'     - pointer to a buffer to hold the incomming bytes
////
//// Returns: number of characters read
////
//int ReadCOM(int inlen, unsigned char *inbuf)
//{
//   DWORD dwLength=0;
//   BOOL fReadStat;
//   DWORD ler=0,to,i;
//
//   // calculate a timeout
//   to = 20 * inlen + 60;
//
//   // reset the read event
//   ResetEvent(osRead.hEvent);
//
//   // read
//   fReadStat = ReadFile(ComID, (LPSTR) &inbuf[0],
//                      inlen, &dwLength, &osRead) ;
//
//   // check for an error
//   if (!fReadStat)
//      ler = GetLastError();
//
//   // if not done writing then wait
//   if (!fReadStat && ler == ERROR_IO_PENDING)
//   {
//      // wait until everything is read
//      WaitForSingleObject(osRead.hEvent,to);
//
//      // verify all is read correctly
//      fReadStat = GetOverlappedResult(ComID, &osRead,
//                   &dwLength, FALSE);
//   }
//
//   // check results
//   if (fReadStat)
//   {
//      // debug
//      if (dodebug)
//      {
//         for (i = 0; i < dwLength; i++)
//            printf("<%02X",inbuf[i]);
//         printf("\n");
//      }
//
//      return dwLength;
//   }
//   else
//      return 0;
//}
//
////--------------------------------------------------------------------------
//// Send a break on the com port for at least 2 ms
////
//void BreakCOM(void)
//{
//   // start the reset pulse，将通信设备挂起，该函数将指定的通信设备暂停字符传输，处于挂起状态，并使传输线路暂处于中断状态
//   SetCommBreak(ComID);
//
//   // sleep
//   Sleep(2);
//
//   // clear the break
//   ClearCommBreak(ComID);//将通信设备从挂起状态恢复
//
//   // debug
//   if (dodebug)
//      printf("__Break__\n");
//}
//
////--------------------------------------------------------------------------
//// Set the baud rate on the com port.
////
//// 'new_baud'  - new baud rate defined as
////                PARMSET_9600     0x00
////                PARMSET_19200    0x02
////                PARMSET_57600    0x04
////                PARMSET_115200   0x06
////
//void SetBaudCOM(unsigned char new_baud)
//{
//   DCB dcb;
//
//   // get the current com port state
//   GetCommState(ComID, &dcb);
//
//   // change just the baud rate
//   switch (new_baud)
//   {
//      case PARMSET_115200:
//         dcb.BaudRate = CBR_115200;
//         break;
//      case PARMSET_57600:
//         dcb.BaudRate = CBR_57600;
//         break;
//      case PARMSET_19200:
//         dcb.BaudRate = CBR_19200;
//         break;
//      case PARMSET_9600:
//      default:
//         dcb.BaudRate = CBR_9600;
//         break;
//   }
//
//   // restore to set the new baud rate
//   SetCommState(ComID, &dcb);
//
//   // debug
//   if (dodebug)
//      PRINT("__SetBaudCOM %d__\n",dcb.BaudRate);
//}
//
////--------------------------------------------------------------------------
////  Description:
////     Delay for at least 'len' ms
////
//void msDelay(int len)
//{
//   // debug
//   if (dodebug)
//      printf("__msDelay %d__\n",len);
//
//   Sleep(len);
//}
//
//
//
//

