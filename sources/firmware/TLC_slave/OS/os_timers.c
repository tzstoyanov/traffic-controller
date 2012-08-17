#include "types.h"
#include "os.h"

/////////////////
// types
/////////////////

/////////////////
// macro
/////////////////

/////////////////
// const
/////////////////

/////////////////
// var
/////////////////
tOSTimer OSSysTimer = 1;


/////////////////
// prototypes
/////////////////

/////////////////////////
// implementation, local
/////////////////////////

////////////////////////////
// implementation, exported
////////////////////////////
void OSTimerTask(void)
{
    OSSysTimer ++;
    if ((tOSTimer)0 == OSSysTimer)
    {
        OSSysTimer++;
    }
}

void OSStartTimer(tOSTimer * Timer)
{
    *Timer = OSSysTimer;
}

void OSStopTimer(tOSTimer * Timer)
{
    *Timer = 0;
}

tOSTimer OSGetElapsedTime(tOSTimer Timer)
{
    tOSTimer Result;

    if ((tOSTimer)0 == Timer)
    {
        Result = 0;
    }
    else
    {
        Result = OSSysTimer - Timer;
        if (Timer > OSSysTimer)
        {
            Result -=1;
        }
    }
    return Result;
}

bool OSIsTimerElapsed(tOSTimer * Timer, tOSTimer Delay)
{
    bool Result = FALSE;
    tOSTimer ElapsedTime;

    if ((tOSTimer)0 == *Timer)
    {   // null timer always elapsed
        Result = TRUE;
    }
    else
    {
        ElapsedTime = OSGetElapsedTime(*Timer);
        if (Delay <= ElapsedTime)
        {
            Result = TRUE;
            *Timer = 0;
        }
    }
    return Result;
}
