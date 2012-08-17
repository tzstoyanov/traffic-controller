#include "types.h"

#include "semihosting.h"
#include "lpc11xx_uart.h"

// open the output stream
void TSTOpen( const char * pStr )
{
}

// close the output stream
void TSTClose( void )
{
}

// write to output stream
void TSTOutStr( const char *pStr )
{
    U8 index = 0;
    while (pStr[index]!=0) index++;
    UART_Send(LPC_UART, pStr, index, BLOCKING);

//    SH_SendString(pStr);

}

u08 Hex[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void TSTOut32H(u32 U)
{ u08 msg[9];
  s32 i;

  for(i=0; i<8; i++)
  {
    msg[7-i]=Hex[U&0x00000000F];
    U>>=4;
  }

  msg[8]=0;
  TSTOutStr(msg);
}

void TSTOut16H(u16 U)
{ u08 msg[5];
  s32 i;

  for(i=0; i<4; i++)
  {
    msg[3-i]=Hex[U&0x00000000F];
    U>>=4;
  }

  msg[4]=0;
  TSTOutStr((const u08 *)msg);
}

void TSTOut8H(u08 U)
{ u08 msg[3];
  s32 i;

  for(i=0; i<2; i++)
  {
    msg[1-i]=Hex[U&0x00000000F];
    U>>=4;
  }

  msg[2]=0;
  TSTOutStr((const u08 *)msg);
}
