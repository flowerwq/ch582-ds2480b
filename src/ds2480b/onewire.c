#include <stdio.h>
#include "ds2480b/commun.h"
#include "ds2480b/uart-ds2480.h"
#include "ds2480b/common.h"
#include "ds2480b/onewire.h"
#include "uart.h"
#include "CH58x_common.h"
#include "log.h"
#include <stdbool.h>
#define TAG "onewire"

// search state
unsigned char ROM_NO[8];
int LastDiscrepancy;
int LastFamilyDiscrepancy;
int LastDeviceFlag;
unsigned char crc8;


// DS2480B state
int ULevel; // 1-Wire level
int UBaud;  // baud rate
int UMode;  // command or data mode state
int USpeed; // 1-Wire communication speed
int ALARM_RESET_COMPLIANCE = FALSE; // flag for DS1994/DS2404 'special' reset

//---------------------------------------------------------------------------
//-------- Basic 1-Wire functions
//---------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Reset all of the devices on the 1-Wire Net and return the result.
//
// Returns: TRUE(1):  presense pulse(s) detected, device(s) reset
//          FALSE(0): no presense pulses detected
//
// WARNING: Without setting the above global (FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE)
//          to TRUE, this routine will not function correctly on some
//          Alarm reset types of the DS1994/DS1427/DS2404 with
//          Rev 1,2, and 3 of the DS2480/DS2480B. 
//          
//
bool OWReset(void)
{
   uint8_t readbuffer[10]={0},sendpacket[10]={0};
   volatile uint8_t sendlen=0;

   // make sure normal level
   OWLevel(MODE_NORMAL);

   // check for correct mode
   if (UMode != MODSEL_COMMAND)
   {
      UMode = MODSEL_COMMAND;
      sendpacket[sendlen++] = MODE_COMMAND;
   }
//   USpeed=SPEEDSEL_FLEX;
   // construct the command
   sendpacket[sendlen++] = (unsigned char)(CMD_COMM | FUNCTSEL_RESET | USpeed);
//   for(int i=0; i<2; i++){
//       PRINT("%2x \r\n",sendpacket[i]);
//   }
    LOG_DEBUG(TAG, "%d bytes send from reset:", sendlen);

    log_buffer_hex(TAG, sendpacket, sendlen, LOG_LEVEL_INFO);
   // flush the buffers
//   FlushCOM();
   uart_rx_flush(UART_NUM_0);
   uart_tx_flush(UART_NUM_0);

   // send the packet

   if(0 == uart_send(UART_NUM_0,sendpacket,sendlen)){
            // read back the 1 byte response
            if(1 == uart_read(UART_NUM_0,readbuffer,1,10)){
//                for(int i = 0; i < 1; i++){
//                    PRINT("readbuffer:%2x \r\n",readbuffer[i]);
//                }
                 LOG_DEBUG(TAG, "%d bytes read from reset:", 1);
                 log_buffer_hex(TAG, readbuffer, 1, LOG_LEVEL_INFO); 
                
                if(ALARM_RESET_COMPLIANCE)
                 {
                    DelayMs(5); // delay 5 ms to give DS1994 enough time
//                    FlushCOM();
                    return TRUE;
                }
                   // make sure this byte looks like a reset byte
                if(((readbuffer[0] & RB_RESET_MASK) == RB_PRESENCE) ||
                    ((readbuffer[0] & RB_RESET_MASK) == RB_ALARMPRESENCE)){
                     return TRUE;
                    
                }
                
            }
            
   }
   DelayUs(4096);                   
   // an error occured so re-sync with DS2480B
   DS2480B_Detect();
   return FALSE;

}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net.
// The parameter 'sendbit' least significant bit is used.
//
// 'sendbit' - 1 bit to send (least significant byte)
//
void OWWriteBit(unsigned char sendbit)
{
   OWTouchBit(sendbit);
}

//--------------------------------------------------------------------------
// Send 8 bits of read communication to the 1-Wire Net and and return the
// result 8 bits read from the 1-Wire Net.
//
// Returns:  8 bits read from 1-Wire Net
//
unsigned char OWReadBit(void)
{
   return OWTouchBit(0x01);
}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and return the
// result 1 bit read from the 1-Wire Net.  The parameter 'sendbit'
// least significant bit is used and the least significant bit
// of the result is the return bit.
//
// 'sendbit' - the least significant bit is the bit to send
//
// Returns: 0:   0 bit read from sendbit
//          1:   1 bit read from sendbit
//
unsigned char OWTouchBit(unsigned char sendbit)
{
   uint8_t readbuffer[10] = {0},sendpacket[10] = {0};
   uint8_t sendlen=0;

   // make sure normal level
   OWLevel(MODE_NORMAL);

   // check for correct mode
   if (UMode != MODSEL_COMMAND)
   {
      UMode = MODSEL_COMMAND;
      sendpacket[sendlen++] = MODE_COMMAND;
   }

   // construct the command
   sendpacket[sendlen] = (sendbit != 0) ? BITPOL_ONE : BITPOL_ZERO;
   sendpacket[sendlen++] |= CMD_COMM | FUNCTSEL_BIT | USpeed;

   // flush the buffers
//   FlushCOM();

   // send the packet

    LOG_DEBUG(TAG, "%d bytes send from OWTouchBit:", sendlen);
    log_buffer_hex(TAG, sendpacket, sendlen, LOG_LEVEL_INFO); 
    if (0 == uart_send(UART_NUM_0,sendpacket,sendlen))
     {
        // read back the response
        if (1 == uart_read(UART_NUM_0,readbuffer,1,1) )
        {

            LOG_DEBUG(TAG, "%d bytes read from OWTouchBit:", 1);
            log_buffer_hex(TAG, readbuffer, 1, LOG_LEVEL_INFO);
           // interpret the response
           if (((readbuffer[0] & 0xE0) == 0x80) &&
               ((readbuffer[0] & RB_BIT_MASK) == RB_BIT_ONE)){
              return 1;
           }
           else{
              return 0;
           }
        }
     }
   // an error occured so re-sync with DS2480B
   DS2480B_Detect();

   return 0;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).
// The parameter 'sendbyte' least significant 8 bits are used.
//
// 'sendbyte' - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same
//
void OWWriteByte(unsigned char sendbyte)
{
   OWTouchByte(sendbyte);
}

//--------------------------------------------------------------------------
// Send 8 bits of read communication to the 1-Wire Net and and return the
// result 8 bits read from the 1-Wire Net.
//
// Returns:  8 bits read from 1-Wire Net
//
unsigned char OWReadByte(void)
{
   return OWTouchByte(0xFF);
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and return the
// result 8 bits read from the 1-Wire Net.  The parameter 'sendbyte'
// least significant 8 bits are used and the least significant 8 bits
// of the result is the return byte.
//
// 'sendbyte' - 8 bits to send (least significant byte)
//
// Returns:  8 bits read from sendbyte
//
unsigned char OWTouchByte(unsigned char sendbyte)
{
   uint8_t  readbuffer[10] = {0},sendpacket[10] = {0};
   uint8_t  sendlen=0;

   // make sure normal level
   OWLevel(MODE_NORMAL);

   // check for correct mode
   if (UMode != MODSEL_DATA)
   {
      UMode = MODSEL_DATA;
      sendpacket[sendlen++] = MODE_DATA;
   }

   // add the byte to send
   sendpacket[sendlen++] = (unsigned char)sendbyte;

   // check for duplication of data that looks like COMMAND mode
   if (sendbyte == (int)MODE_COMMAND) {
      sendpacket[sendlen++] = (unsigned char)sendbyte;
   }

   // flush the buffers
//   FlushCOM();
    uart_tx_flush(UART_NUM_0);
    uart_rx_flush(UART_NUM_0);
   // send the packet

    if (0 == uart_send(UART_NUM_0,sendpacket,sendlen))
   {
      // read back the 1 byte response
      if (1 == uart_read(UART_NUM_0,readbuffer,1,10))
      {
          // return the response
          return readbuffer[0];
      }
   }
   // an error occured so re-sync with DS2480B
   DS2480B_Detect();

   return 0;
}

//--------------------------------------------------------------------------
// The 'OWBlock' transfers a block of data to and from the
// 1-Wire Net. The result is returned in the same buffer.
//
// 'tran_buf' - pointer to a block of unsigned
//              chars of length 'tran_len' that will be sent
//              to the 1-Wire Net
// 'tran_len' - length in bytes to transfer
//
// Returns:   TRUE (1) : If the buffer transfer was succesful.
//            FALSE (0): If an error occured.
//
//  The maximum tran_length is (160)
//
int OWBlock(unsigned char *tran_buf, int tran_len)
{
   uint8_t sendpacket[320] = {0};
   uint8_t sendlen=0;
   uint8_t pos;

   // check for a block too big
   if (tran_len > 160){
      return FALSE;
   }

   // construct the packet to send to the DS2480B
   // check for correct mode
   if (UMode != MODSEL_DATA)
   {
      UMode = MODSEL_DATA;
      sendpacket[sendlen++] = MODE_DATA;
   }

   // add the bytes to send
   pos = sendlen;
   for (int i = 0; i < tran_len; i++)
   {
      sendpacket[sendlen++] = tran_buf[i];

      // duplicate data that looks like COMMAND mode
      if (tran_buf[i] == MODE_COMMAND){
          sendpacket[sendlen++] = tran_buf[i];
      }
   }

   // flush the buffers
//   FlushCOM();
    uart_tx_flush(UART_NUM_0);
    uart_rx_flush(UART_NUM_0);

   // send the packet

   
    if (0 == uart_send(UART_NUM_0,sendpacket,sendlen))
       {
          // read back the response
          if (tran_len == uart_read(UART_NUM_0,tran_buf,tran_len,10)){
             return TRUE;
          }
       }
// an error occured so re-sync with DS2480B
    DS2480B_Detect();

    return FALSE;
}

//--------------------------------------------------------------------------
// Find the 'first' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : no device present
//
int OWFirst()
{
   // reset the search state
   LastDiscrepancy = 0;
   LastDeviceFlag = FALSE;
   LastFamilyDiscrepancy = 0;

   return OWSearch();
}

//--------------------------------------------------------------------------
// Find the 'next' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
int OWNext()
{
   // leave the search state alone
   return OWSearch();
}

//--------------------------------------------------------------------------
// Verify the device with the ROM number in ROM_NO buffer is present.
// Return TRUE  : device verified present
//        FALSE : device not present
//
int OWVerify()
{
   unsigned char rom_backup[8];
   int i,rslt,ld_backup,ldf_backup,lfd_backup;

   // keep a backup copy of the current state
   for (i = 0; i < 8; i++)
      rom_backup[i] = ROM_NO[i];
   ld_backup = LastDiscrepancy;
   ldf_backup = LastDeviceFlag;
   lfd_backup = LastFamilyDiscrepancy;

   // set search to find the same device
   LastDiscrepancy = 64;
   LastDeviceFlag = FALSE;

   if (OWSearch())
   {
      // check if same device found
      rslt = TRUE;
      for (i = 0; i < 8; i++)
      {
         if (rom_backup[i] != ROM_NO[i])
         {
            rslt = FALSE;
            break;
         }
      }
   }
   else
     rslt = FALSE;

   // restore the search state 
   for (i = 0; i < 8; i++)
      ROM_NO[i] = rom_backup[i];
   LastDiscrepancy = ld_backup;
   LastDeviceFlag = ldf_backup;
   LastFamilyDiscrepancy = lfd_backup;

   // return the result of the verify
   return rslt;
}

//--------------------------------------------------------------------------
// Setup the search to find the device type 'family_code' on the next call
// to OWNext() if it is present.
//
void OWTargetSetup(unsigned char family_code)
{
   int i;

   // set the search state to find SearchFamily type devices
   ROM_NO[0] = family_code;
   for (i = 1; i < 8; i++)
      ROM_NO[i] = 0;
   LastDiscrepancy = 64;
   LastFamilyDiscrepancy = 0;
   LastDeviceFlag = FALSE;
}

//--------------------------------------------------------------------------
// Setup the search to skip the current device type on the next call
// to OWNext().
//
void OWFamilySkipSetup()
{
   // set the Last discrepancy to last family discrepancy
   LastDiscrepancy = LastFamilyDiscrepancy;

   // clear the last family discrpepancy
   LastFamilyDiscrepancy = 0;

   // check for end of list
   if (LastDiscrepancy == 0) 
      LastDeviceFlag = TRUE;

}

//--------------------------------------------------------------------------
// The 'OWSearch' function does a general search.  This function
// continues from the previos search state. The search state
// can be reset by using the 'OWFirst' function.
// This function contains one parameter 'alarm_only'.
// When 'alarm_only' is TRUE (1) the find alarm command
// 0xEC is sent instead of the normal search command 0xF0.
// Using the find alarm command 0xEC will limit the search to only
// 1-Wire devices that are in an 'alarm' state.
//
// Returns:   TRUE (1) : when a 1-Wire device was found and it's
//                       Serial Number placed in the global ROM 
//            FALSE (0): when no new device was found.  Either the
//                       last search was the last device or there
//                       are no devices on the 1-Wire Net.
//
//unsigned char ROM_NO[8];
int OWSearch(void)
{
   unsigned char last_zero,pos;
   uint8_t readbuffer[20]={0},sendpacket[40]={0};
   volatile uint8_t sendlen = 0;
   int search_result = 0;
   PRINT("search start:%s\r\n",__FUNCTION__);
   // if the last call was the last one
   if (LastDeviceFlag)
   {
      // reset the search
      LastDiscrepancy = 0;
      LastDeviceFlag = FALSE;
      LastFamilyDiscrepancy = 0;
      return FALSE;
   }

   // reset the 1-wire
   // if there are no parts on 1-wire, return FALSE
   if (!OWReset())
   {
      // reset the search
      LastDiscrepancy = 0;
      LastFamilyDiscrepancy = 0;
      return FALSE;
   }

   // build the command stream
   // call a function that may add the change mode command to the buff
   // check for correct mode
   if (UMode != MODSEL_DATA)
   {
      UMode = MODSEL_DATA;
      sendpacket[sendlen++] = MODE_DATA;
   }

   // search command
   sendpacket[sendlen++] = 0xF0;
   
   // change back to command mode
   UMode = MODSEL_COMMAND;
   sendpacket[sendlen++] = MODE_COMMAND;

   // search mode on
   sendpacket[sendlen++] = (unsigned char)(CMD_COMM | FUNCTSEL_SEARCHON | USpeed);

   // change back to data mode
   UMode = MODSEL_DATA;
   sendpacket[sendlen++] = MODE_DATA;

   // set the temp Last Discrepancy to 0
   last_zero = 0;
   
   // add the 16 bytes of the search
   pos = sendlen;
   for (int i = 0; i < 16; i++){
      sendpacket[sendlen++] = 0;
   }

   // only modify bits if not the first search
   if (LastDiscrepancy != 0)
   {
      // set the bits in the added buffer
      for (int i = 0; i < 64; i++)
      {
         // before last discrepancy
         if (i < (LastDiscrepancy - 1)){
               bitacc(WRITE_FUNCTION,bitacc(READ_FUNCTION, 0, i,&ROM_NO[0]),(short)
               (i * 2 + 1),&sendpacket[pos]);
         }
         // at last discrepancy
         else if (i == (LastDiscrepancy - 1)){
                bitacc(WRITE_FUNCTION, 1, (short)(i * 2 + 1), &sendpacket[pos]);
         }
         // after last discrepancy so leave zeros
      }
   }

   // change back to command mode
   UMode = MODSEL_COMMAND;
   sendpacket[sendlen++] = MODE_COMMAND;

   // search OFF command
   sendpacket[sendlen++] = (unsigned char)(CMD_COMM | FUNCTSEL_SEARCHOFF | USpeed);

   // flush the buffers
//   FlushCOM();
   uart_rx_flush(UART_NUM_0);
   uart_tx_flush(UART_NUM_0);

//   for(int i = 0; i < sendlen; i++){
//        PRINT("sendpack:%2x \r\n",sendpacket[i]);
//   }

   LOG_DEBUG(TAG, "%d bytes send from search:", sendlen);
   log_buffer_hex(TAG, sendpacket, sendlen, LOG_LEVEL_INFO); 
   // send the packet
   if (0 == uart_send(UART_NUM_0,sendpacket,sendlen))
   {
      // read back the 17 byte response
      if (17 == uart_read(UART_NUM_0,readbuffer,17,1000))
       {
//         for(int a = 0; a < 17; a++){
//            PRINT("read:%2x \r\n",readbuffer[a]);
//        }

         LOG_DEBUG(TAG, "%d bytes read from search:", 17);
         log_buffer_hex(TAG, readbuffer, 17, LOG_LEVEL_INFO); 
         // interpret the bit stream
         for (int i = 0; i < 64; i++)
         {
            // get the ROM bit
            bitacc(WRITE_FUNCTION,bitacc(READ_FUNCTION,0,(short)(i * 2 + 1),&readbuffer[1])
            , i, &ROM_NO[0]);
            // check LastDiscrepancy
            if ((bitacc(READ_FUNCTION,0,(short)(i * 2),&readbuffer[1]) == 1) &&
                (bitacc(READ_FUNCTION,0,(short)(i * 2 + 1),&readbuffer[1]) == 0))
            {
               last_zero = i + 1;
               // check LastFamilyDiscrepancy
               if (i < 8){
                  LastFamilyDiscrepancy = i + 1;
               }
            }
         }
         
         LOG_DEBUG(TAG, "tem_rom:");
         log_buffer_hex(TAG, ROM_NO, sizeof(ROM_NO), LOG_LEVEL_INFO);

//         if(docrc8(ROM_NO) == &ROM_NO){
//            
//         }
         // do dowcrc
         crc8 = 0;
//         for (int i = 0; i < 8; i++){
//            docrc8(ROM_NO[i]);
//         }

         // check results
         if ((crc8 != 0) || (LastDiscrepancy == 63) || (ROM_NO[0] == 0))
         {
            // error during search
            // reset the search
            LastDiscrepancy = 0;
            LastDeviceFlag = FALSE;
            LastFamilyDiscrepancy = 0;
            search_result=FALSE;
            PRINT("find error\r\n");
         }
         // successful search
         else
         {
            // set the last discrepancy
            LastDiscrepancy = last_zero;

            // check for last device
            if (LastDiscrepancy == 0){
               LastDeviceFlag = TRUE;
            }

            // copy the ROM to the buffer
//            for (volatile int i = 0; i < 8; i++){
//               ROM_NO[i] = ROM_NO[i];
//            }
            search_result=TRUE;
         }
      }
   }

   
   
   // an error occured so re-sync with DS2480B
   if(DS2480B_Detect()  == FALSE){

       // reset the search
       LastDiscrepancy = 0;
       LastDeviceFlag = FALSE;
       LastFamilyDiscrepancy = 0;
       search_result=FALSE;      

   }
   PRINT("search finish:%s\r\n",__FUNCTION__);
   return search_result;
}


int onewire_search()
{
   int id_bit_number;
   int last_zero, rom_byte_number, search_result;
   int id_bit, cmp_id_bit;
   unsigned char rom_byte_mask, search_direction;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = 0;
   crc8 = 0;

   // if the last call was not the last one
   if (!LastDeviceFlag){
      // 1-Wire reset
      if (!OWReset()){
         // reset the search
         LastDiscrepancy = 0;
         LastDeviceFlag = FALSE;
         LastFamilyDiscrepancy = 0;
         return FALSE;
      }

      // issue the search command 
      OWWriteByte(0xF0);  //搜寻总线上的所有从机设备

      // loop to do the search
      do
      {
         // read a bit and its complement
         id_bit = OWReadBit();
         cmp_id_bit = OWReadBit();

         // check for no devices on 1-wire
         if ((id_bit == 1) && (cmp_id_bit == 1)){
		 	PRINT("no device exist\r\n");
            break;
		 }
         else
         {
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit){
               search_direction = id_bit;  // bit write value for search
            }
            else
            {
               // if this discrepancy if before the Last Discrepancy
               // on a previous next then pick the same as last time
               if (id_bit_number < LastDiscrepancy){
                  search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
			   }
               else{
                  // if equal to last pick 1, if not then pick 0
                  search_direction = (id_bit_number == LastDiscrepancy);
			   }

               // if 0 was picked then record its position in LastZero
               if (search_direction == 0)
               {
                  last_zero = id_bit_number;

                  // check for Last discrepancy in family
                  if (last_zero < 9){
                     LastFamilyDiscrepancy = last_zero;
				  }
               }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1){
              ROM_NO[rom_byte_number] |= rom_byte_mask;
            }
            else{
              ROM_NO[rom_byte_number] &= ~rom_byte_mask;
            }

            // serial number search direction write bit
            OWWriteBit(search_direction);

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0)
            {
                //crc8 = onewire_crc8(ROM_NO[rom_byte_number],sizeof(ROM_NO[rom_byte_number]));  // accumulate the CRC
                rom_byte_number++;
                rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

      // if the search was successful then
      if (!((id_bit_number < 65) || (crc8 != 0)))	//id_bit_number  >= 65 && crc == 0
      {
         // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
         LastDiscrepancy = last_zero;

         // check for last device
         if (LastDiscrepancy == 0){
            LastDeviceFlag = TRUE;
		 }
         
         search_result = TRUE;
      }
   }

   // if no device found then reset counters so next 'search' will be like a first
   if (!search_result || !ROM_NO[0])
   {
      LastDiscrepancy = 0;
      LastDeviceFlag = FALSE;
      LastFamilyDiscrepancy = 0;
      search_result = FALSE;
	  PRINT("search finish.\r\n");
	  __nop();
   }

   return search_result;
}


//---------------------------------------------------------------------------
//-------- Extended 1-Wire functions
//---------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Set the 1-Wire Net communucation speed.
//
// 'new_speed' - new speed defined as
//                MODE_NORMAL     0x00
//                MODE_OVERDRIVE  0x01
//
// Returns:  current 1-Wire Net speed
//
//int OWSpeed(int new_speed)
//{
//   unsigned char sendpacket[5];
//   unsigned char sendlen=0;
//   unsigned char rt = FALSE;
//
//   // check if change from current mode
//   if (((new_speed == MODE_OVERDRIVE) &&
//        (USpeed != SPEEDSEL_OD)) ||
//       ((new_speed == MODE_NORMAL) &&
//        (USpeed != SPEEDSEL_FLEX)))
//   {
//      if (new_speed == MODE_OVERDRIVE)
//      {
//         // if overdrive then switch to 115200 baud
//         if (DS2480B_ChangeBaud(MAX_BAUD) == MAX_BAUD)
//         {
//            USpeed = SPEEDSEL_OD;
//            rt = TRUE;
//         }
//
//      }
//      else if (new_speed == MODE_NORMAL)
//      {
//         // else normal so set to 9600 baud
//         if (DS2480B_ChangeBaud(PARMSET_9600) == PARMSET_9600)
//         {
//            USpeed = SPEEDSEL_FLEX;
//            rt = TRUE;
//         }
//
//      }
//
//      // if baud rate is set correctly then change DS2480B speed
//      if (rt)
//      {
//         // check for correct mode
//         if (UMode != MODSEL_COMMAND)
//         {
//            UMode = MODSEL_COMMAND;
//            sendpacket[sendlen++] = MODE_COMMAND;
//         }
//
//         // proceed to set the DS2480B communication speed
//         sendpacket[sendlen++] = CMD_COMM | FUNCTSEL_SEARCHOFF | USpeed;
//
//         // send the packet
//         if (!WriteCOM(sendlen,sendpacket))
//         {
//            rt = FALSE;
//            // lost communication with DS2480B then reset
//            DS2480B_Detect();
//         }
//      }
//
//   }
//
//   // return the current speed
//   return (USpeed == SPEEDSEL_OD) ? MODE_OVERDRIVE : MODE_NORMAL;
//}

//--------------------------------------------------------------------------
// Set the 1-Wire Net line level.  The values for new_level are
// as follows:
//
// 'new_level' - new level defined as
//                MODE_NORMAL     0x00
//                MODE_STRONG5    0x02
//
// Returns:  current 1-Wire Net level
//
int OWLevel(int new_level)
{
    uint8_t sendpacket[10] = {0},readbuffer[10] = {0};
    uint8_t sendlen = 0;
    unsigned char rt = FALSE;
    unsigned char docheck = FALSE;

   // check if need to change level
   if (new_level != ULevel)
   {
      // check for correct mode
      if (UMode != MODSEL_COMMAND)
      {
         UMode = MODSEL_COMMAND;
         sendpacket[sendlen++] = MODE_COMMAND;
      }
      
      // check if just putting back to normal
      if (new_level == MODE_NORMAL)
      {
         // check for disable strong pullup step
         if (ULevel == MODE_STRONG5){
            docheck = TRUE;
         }

         // stop pulse command
         sendpacket[sendlen++] = MODE_STOP_PULSE;

         // add the command to begin the pulse WITHOUT prime
         sendpacket[sendlen++] = CMD_COMM | FUNCTSEL_CHMOD | SPEEDSEL_PULSE | BITPOL_5V | PRIME5V_FALSE;

         // stop pulse command
         sendpacket[sendlen++] = MODE_STOP_PULSE;

         // flush the buffers
//         FlushCOM();

         // send the packet

         if(0 == uart_send(UART_NUM_0,sendpacket,sendlen)){
            if(2 == uart_read(UART_NUM_0,readbuffer,2,10)){
                if(((readbuffer[0] & 0xE0) == 0xE0) && ((readbuffer[1] & 0xE0) == 0xE0)){
                    rt=TRUE;
                    ULevel = MODE_NORMAL;
                    
                }
                
            }
         }
      }
      // set new level
      else
      {
         // set the SPUD time value
         sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_5VPULSE | PARMSET_infinite;
         // add the command to begin the pulse
         sendpacket[sendlen++] = CMD_COMM | FUNCTSEL_CHMOD | SPEEDSEL_PULSE | BITPOL_5V;

         // flush the buffers
//         FlushCOM();

         // send the packet
         if(0 == uart_send(UART_NUM_0,sendpacket,sendlen)){
            if(1 == uart_read(UART_NUM_0,readbuffer,1,10)){
                if((readbuffer[0] & 0x81) == 0){
                    ULevel = new_level;
                    rt=TRUE;
                    
                }
                
            }
         }
      }
         
      // if lost communication with DS2480B then reset
      if (rt != TRUE){
         DS2480B_Detect();
      }
   }

   // return the current level
   return ULevel;
}

//--------------------------------------------------------------------------
// This procedure creates a fixed 480 microseconds 12 volt pulse
// on the 1-Wire Net for programming EPROM iButtons.
//
// Returns:  TRUE  successful
//           FALSE program voltage not available
//
int OWProgramPulse(void)
{
    unsigned char sendpacket[10],readbuffer[10];
    unsigned char sendlen=0;
// make sure normal level
    OWLevel(MODE_NORMAL);

// check for correct mode
    if (UMode != MODSEL_COMMAND)
    {
      UMode = MODSEL_COMMAND;
      sendpacket[sendlen++] = MODE_COMMAND;
    }

// set the SPUD time value
    sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_12VPULSE | PARMSET_512us;

// pulse command
    sendpacket[sendlen++] = CMD_COMM | FUNCTSEL_CHMOD | BITPOL_12V | SPEEDSEL_PULSE;

// flush the buffers
    uart_tx_flush(UART_NUM_0);
    uart_tx_flush(UART_NUM_0);

// send the packet
   if (uart_write(sendpacket,sendlen))
   {
// read back the 2 byte response
      if (uart_read(UART_NUM_0,readbuffer,2,1) == 2)
      {
         // check response byte
         if (((readbuffer[0] | CMD_CONFIG) ==
                (CMD_CONFIG | PARMSEL_12VPULSE | PARMSET_512us)) &&
             ((readbuffer[1] & 0xFC) ==
                (0xFC & (CMD_COMM | FUNCTSEL_CHMOD | BITPOL_12V | SPEEDSEL_PULSE)))){
                   return TRUE;
         }
      }
   }

   // an error occured so re-sync with DS2480B
   DS2480B_Detect();

   return FALSE;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).  
// The parameter 'sendbyte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
//
// 'sendbyte' - 8 bits to send (least significant bit)
//
// Returns:  TRUE: bytes written and echo was the same, strong pullup now on
//           FALSE: echo was not the same 
//
int OWWriteBytePower(int sendbyte)
{
    unsigned char sendpacket[10] = {0},readbuffer[10] = {0};
    unsigned char sendlen=0;
    unsigned char rt=FALSE;
    unsigned char i, temp_byte;

    // check for correct mode
    if (UMode != MODSEL_COMMAND)
    {
        UMode = MODSEL_COMMAND;
        sendpacket[sendlen++] = MODE_COMMAND;
    }

    // set the SPUD time value
    sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_5VPULSE | PARMSET_infinite;

    // construct the stream to include 8 bit commands with the last one
    // enabling the strong-pullup
    
    temp_byte = sendbyte;
    for (i = 0; i < 8; i++)
    {
      sendpacket[sendlen++] = ((temp_byte & 0x01) ? BITPOL_ONE : BITPOL_ZERO)
                              | CMD_COMM | FUNCTSEL_BIT | USpeed |
                              ((i == 7) ? PRIME5V_TRUE : PRIME5V_FALSE);
      temp_byte >>= 1;
    }

    // flush the buffers
    uart_tx_flush(UART_NUM_0);
    uart_rx_flush(UART_NUM_0);

// send the packet
    if (0 == uart_send(UART_NUM_0,sendpacket,sendlen))
    {
      // read back the 9 byte response from setting time limit
      if (9 == uart_read(UART_NUM_0,readbuffer,9,1))
      {
         // check response 
         if ((readbuffer[0] & 0x81) == 0)
         {
            // indicate the port is now at power delivery
            ULevel = MODE_STRONG5;

            // reconstruct the echo byte
            temp_byte = 0; 
            for (i = 0; i < 8; i++)
            {
               temp_byte >>= 1;
               temp_byte |= (readbuffer[i + 1] & 0x01) ? 0x80 : 0;
            }
            
            if (temp_byte == sendbyte){
               rt = TRUE;
            }
         }
      }
    }

    // if lost communication with DS2480B then reset
    if (rt != TRUE){
      DS2480B_Detect();
    }

    return rt;
}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and verify that the
// response matches the 'applyPowerResponse' bit and apply power delivery
// to the 1-Wire net.  Note that some implementations may apply the power
// first and then turn it off if the response is incorrect.
//
// 'applyPowerResponse' - 1 bit response to check, if correct then start
//                        power delivery 
//
// Returns:  TRUE: bit written and response correct, strong pullup now on
//           FALSE: response incorrect
//
int OWReadBitPower(int applyPowerResponse)
{
    unsigned char sendpacket[3] = {0},readbuffer[3] = {0};
    unsigned char sendlen=0;
    unsigned char rt=FALSE;

    // check for correct mode
    if (UMode != MODSEL_COMMAND)
    {
        UMode = MODSEL_COMMAND;
        sendpacket[sendlen++] = MODE_COMMAND;
    }

    // set the SPUD time value
    sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_5VPULSE | PARMSET_infinite;

    // enabling the strong-pullup after bit
    sendpacket[sendlen++] = BITPOL_ONE 
                           | CMD_COMM | FUNCTSEL_BIT | USpeed |
                           PRIME5V_TRUE;
    // flush the buffers
    uart_tx_flush(UART_NUM_0);
    uart_rx_flush(UART_NUM_0);
    
    // send the packet
    if (0 == uart_send(UART_NUM_0,sendpacket,sendlen))
    {
      // read back the 2 byte response from setting time limit
      if (2 == uart_read(UART_NUM_0,readbuffer,2,10))
      {
         // check response to duration set
         if ((readbuffer[0] & 0x81) == 0)
         {
            // indicate the port is now at power delivery
            ULevel = MODE_STRONG5;

            // check the response bit
            if ((readbuffer[1] & 0x01) == applyPowerResponse){
                rt = TRUE;
            }
            else{
                OWLevel(MODE_NORMAL);
            }

            return rt;
         }
      }
    }

    // if lost communication with DS2480B then reset
    if (rt != TRUE){
      DS2480B_Detect();
    }

    return rt;
}


// TEST BUILD
static unsigned char dscrc_table[] = {
        0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
      157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
       35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
      190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
       70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
      219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
      101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
      248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
      140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
       17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
      175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
       50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
      202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
       87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
      233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
      116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current 
// global 'crc8' value. 
// Returns current global crc8 value
//
unsigned char docrc8(unsigned char value)
{
   // See Application Note 27
   
   // TEST BUILD
   crc8 = dscrc_table[crc8 ^ value];
   return crc8;
}

//---------------------------------------------------------------------------
//-------- DS2480B functions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Attempt to resyc and detect a DS2480B and set the FLEX parameters
//
// Returns:  TRUE  - DS2480B detected successfully
//           FALSE - Could not detect DS2480B
//
int DS2480B_Detect(void)
{
   uint8_t  sendpacket[10]={0},readbuffer[10]={0};
   uint8_t  sendlen=0;

   // reset modes
   UMode = MODSEL_COMMAND;
   UBaud = PARMSET_9600;
   USpeed = SPEEDSEL_FLEX;

   PRINT("detect start:%s\r\n",__FUNCTION__);
   // set the baud rate to 9600
//   change_baud_rate(UBaud);

   // send a break to reset the DS2480B
   uart_send_break(UART_NUM_0,10);

   // delay to let line settle
   DelayMs(2);


   // send the timing byte
   sendpacket[0] = 0xC1;
   if ( uart_send(UART_NUM_0,sendpacket,1) != 0){
      return FALSE;
   }
  

   // delay to let line settle
   DelayMs(2);

   // set the FLEX configuration parameters
   // default PDSRC = 1.37Vus
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_SLEW | PARMSET_Slew1p37Vus;
   // default W1LT = 10us
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_WRITE1LOW | PARMSET_Write10us;
   // default DSO/WORT = 8us
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_SAMPLEOFFSET | PARMSET_SampOff8us;

   // construct the command to read the baud rate (to test command block)
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_PARMREAD | (PARMSEL_BAUDRATE >> 3);

   // also do 1 bit operation (to test 1-Wire block)
   sendpacket[sendlen++] = CMD_COMM | FUNCTSEL_BIT | UBaud | BITPOL_ONE;

//   // flush the buffers
//   FlushCOM();
   uart_tx_flush(UART_NUM_0);
   uart_rx_flush(UART_NUM_0);

    LOG_DEBUG(TAG, "%d bytes send from ds2480b_detect:",sendlen);
    log_buffer_hex(TAG, sendpacket, sendlen, LOG_LEVEL_INFO);    
   // send the packet
   if(0 == uart_send(UART_NUM_0,sendpacket,sendlen)){
        if(5 == uart_read(UART_NUM_0,readbuffer,5,10)){
            
            LOG_DEBUG(TAG, "%d bytes read from search:", 5);
            log_buffer_hex(TAG, readbuffer, 5, LOG_LEVEL_INFO); 
            if (((readbuffer[3] & 0xF1) == 0x00) &&
             ((readbuffer[3] & 0x0E) == UBaud) &&
             ((readbuffer[4] & 0xF0) == 0x90) &&
             ((readbuffer[4] & 0x0C) == UBaud)){
                return TRUE;
            }

        }

    }
   else{

       return FALSE;
   }
   return 0;
   PRINT("detect finish:%s\r\n",__FUNCTION__);
}

//---------------------------------------------------------------------------
// Change the DS2480B from the current baud rate to the new baud rate.
//
// 'newbaud' - the new baud rate to change to, defined as:
//               PARMSET_9600     0x00
//               PARMSET_19200    0x02
//               PARMSET_57600    0x04
//               PARMSET_115200   0x06
//
// Returns:  current DS2480B baud rate.
int DS2480B_ChangeBaud(unsigned char newbaud)
{
   unsigned char rt=FALSE;
   uint8_t readbuffer[5] = {0},sendpacket[5] = {0};
   uint8_t sendpacket2[5] = {0};
   unsigned char sendlen=0,sendlen2=0;

    //see if diffenent then current baud rate
   if (UBaud == newbaud){
        return UBaud;
   }else
   {
//       build the command packet
//       check for correct mode
        if (UMode != MODSEL_COMMAND)
        {
         UMode = MODSEL_COMMAND;
         sendpacket[sendlen++] = MODE_COMMAND;
        }
//       build the command
        sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_BAUDRATE | newbaud;

//    flush the buffers
//      FlushCOM();
        uart_tx_flush(UART_NUM_0);
        uart_rx_flush(UART_NUM_0);

//       send the packet
        if (uart_send(UART_NUM_0,sendpacket,sendlen) != 0){
            rt = FALSE;
        }else
        {
//          make sure buffer is flushed
            DelayMs(5);

//          change our baud rate
//            SetBaudCOM(newbaud);
            uart_set_baudrate(UART_NUM_0,newbaud);

            UBaud = newbaud;

//          wait for things to settle
            DelayMs(5);

//          build a command packet to read back baud rate
            sendpacket2[sendlen2++] = CMD_CONFIG | PARMSEL_PARMREAD | (PARMSEL_BAUDRATE >> 3);

//          flush the buffers
//            FlushCOM();
            uart_tx_flush(UART_NUM_0);
            uart_rx_flush(UART_NUM_0);

//          send the packet
            if (0 == uart_send(UART_NUM_0,sendpacket2,sendlen2))
            {
//             read back the 1 byte response
                if (1 == uart_read(UART_NUM_0,readbuffer,1,10))
                {
//                verify correct baud
                   if (((readbuffer[0] & 0x0E) == (sendpacket[sendlen-1] & 0x0E))){
                      rt = TRUE;
                   }
                }
             }
        }
   }

//    if lost communication with DS2480B then reset
   if (rt != TRUE){
      DS2480B_Detect();
   }

   return UBaud;
}





