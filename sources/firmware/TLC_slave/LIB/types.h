//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------

#ifndef _TYPES_H_
 #define _TYPES_H_

#include "lpc_types.h"

    typedef unsigned char  u08;
    typedef unsigned short u16;
    typedef unsigned long  u32;
    typedef signed   char  s08;
    typedef signed   short s16;
    typedef signed   long  s32;
    typedef unsigned char  bool;

    typedef u08 U8;
    typedef u16 U16;
    typedef u32 U32;

    typedef s08 S8;
    typedef s16 S16;
    typedef s32 S32;

    //#define TRUE   (0==0)
    //#define FALSE  (0!=0)

#ifndef NULL
    #define NULL    ((void*)0)
#endif

    typedef void (*tpFunc) (void);
#endif
