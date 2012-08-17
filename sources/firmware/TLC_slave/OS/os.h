// OS interface

#ifndef OS_H
 #define OS_H

    #include "system.h"
    #include "types.h"
    #include "OS_cfg.h"

    // do not cast!
    #define cOSSlotSize    8
    #define cOSTaskSize    16

    #define mOSEnterCriticalSection()      \
    { U32 IT = __get_PRIMASK();            \
      __set_PRIMASK(1);

    #define mOSLeaveCriticalSection()      \
      __set_PRIMASK(IT);                   \
    }

    ///////////////

    #if cOSTaskSize == 8
        typedef U8 tOSTaskID;
    #elif cOSTaskSize == 16
        typedef U16 tOSTaskID;
    #elif cOSTaskSize == 32
        typedef U32 tOSTaskID;
    #else
        #error "TaskSize not well defined"
    #endif

    typedef U32 tOSTimer;
    extern tOSTimer OSSysCounter;

    // attach on timer for base tick
    void OSTick(void);
    // Task functions
    void OSActivateTask(tOSTaskID TaskID);

    // timers
    void OSStartTimer(tOSTimer * Timer);
    void OSStopTimer(tOSTimer * Timer);
    tOSTimer OSGetElapsedTime(tOSTimer Timer);
    bool OSIsTimerElapsed(tOSTimer * Timer, tOSTimer Delay);

    // call-backs
    extern void OSStarupHook(void);

#endif
