#include "ds2480b/commun.h"
#include "ds2480b/uart-ds2480.h"
#include "ds2480b/common.h"
#include "ds2480b/onewire.h"

//---------------------------------------------------------------------------
//-------- Utility functions
//---------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Bit utility to read and write a bit in the buffer 'buf'.
//
// 'op'    - operation (1) to set and (0) to read
// 'state' - set (1) or clear (0) if operation is write (1)
// 'loc'   - bit number location to read or write
// 'buf'   - pointer to array of bytes that contains the bit
//           to read or write
//
// Returns: 1   if operation is set (1)
//          0/1 state of bit number 'loc' if operation is reading
//
int bitacc(int op, int state, int loc, uint8_t *buf)
{
   int nbyt = 0;
   int nbit = 0;

   nbyt = (loc / 8);
   nbit = loc - (nbyt * 8);

   if (op == WRITE_FUNCTION)
   {
      if (state){
         buf[nbyt] |= (0x01 << nbit);
      }
      else{
         buf[nbyt] &= ~(0x01 << nbit);
      }

      return 1;
   }
   else{
      return ((buf[nbyt] >> nbit) & 0x01);
   }
}

int find_device(){
    volatile int i;
    volatile int cnt = 0;
    volatile int rslt =0;
    extern unsigned char ROM_NO[8] ;

    PRINT("\nFIND ALL\r\n");

    rslt = OWFirst();
    while (rslt)
    {
      // print device found
      for (i = 0; i < 8; i++){
         PRINT("%02X ", ROM_NO[i]);
      }
      PRINT("序号为： %d\r\n",++cnt);

      rslt = OWNext();
    }
    return 0;
}


