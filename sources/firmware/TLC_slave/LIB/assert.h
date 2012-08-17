#ifndef ASSERT_H
#define ASSERT_H

#include "system.h"

    #ifdef DEBUG_ASSERT
        #ifdef SILENT_ASSERT
            #include "tst.h"
            #define Assert(COND, TEXT)    if(!(COND)) {TSTOutStr("\nAssert:"); TSTOutStr(__FUNCTION__);TSTOutStr(":"); TSTOutStr(TEXT);} else {}
        #else
            #define Assert(COND, TEXT)    while(1);
        #endif
    #else
        #define Assert(COND, TEXT)
    #endif
#endif
