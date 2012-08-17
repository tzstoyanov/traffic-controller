/* Implementation of basic task engine
 *
 */

#include "types.h"
#include "os.h"

/////////////////
// types
/////////////////
    #if cOSSlotSize == 8
        typedef U8 tOSTaskCounter;
    #elif cOSSlotSize == 16
        typedef U16 tOSTaskCounter;
    #elif cOSSlotSize == 32
        typedef U32 tOSTaskCounter;
    #else
        #error "SlotSize not well defined"
    #endif


/////////////////
// macro
/////////////////

/////////////////
// const
/////////////////
static const tpFunc OSTaskCallbacks[cOSTaskSize] = mOSTaskCallbacksInitilizer();

/////////////////
// var
/////////////////
static tOSTaskCounter OSTaskCounter;
// TODO add an initializer ?!?
static tOSTaskID OSTasksList[cOSSlotSize];

tOSTaskID OSActiveTasks;
tOSTaskID OSCurrentTasks;

/////////////////
// prototypes
/////////////////
static U8 OSGetSlot(tOSTaskCounter Counter);

/////////////////////////
// implementation, local
/////////////////////////
static U8 OSGetSlot(tOSTaskCounter Counter)
{
    // TODO: try to use a table?!?
    U8 Slot = 0;
    while ((0 == (Counter & 0x01)) && (Slot < cOSSlotSize))
    {
        Counter >>=1;
        Slot++;
    }
    return Slot;
}

static void OSInit(void)
{
    OSStarupHook();
}

////////////////////////////
// implementation, exported
////////////////////////////
void OSTick(void)
{
    OSTaskCounter++;
    if (0 != OSTaskCounter)
    { // TODO: add a measure to test the slot activation delays ?!?
        OSActiveTasks |= OSTasksList[OSGetSlot(OSTaskCounter)];
    }
}

void OSActivateTask(tOSTaskID TaskID)
{
    OSActiveTasks |= TaskID;
}

void OSDeactivateTask(tOSTaskID TaskID)
{
    OSActiveTasks &= TaskID;
}

void OSSetPeriodicAlarm(tOSTaskID TaskID, U8 Slot)
{
    if (Slot < cOSSlotSize)
    {
        OSTasksList[Slot] |= TaskID;
    }
    else
    { //assert?!?
    }
}

void OSStopPeriodicAlarm(tOSTaskID TaskID)
{
    U8 Slot;
    for (Slot = 0; Slot < cOSSlotSize; Slot++)
    {
        OSTasksList[Slot] &= (U8)~TaskID;
    }
}

// void OSStart
void OSStart(void)
{
    tOSTaskID TempTaskWord;
    tpFunc pCallback;
    U8 TaskNo;

    OSInit();

    //__EI();
    __set_PRIMASK(0);
    while(TRUE)
    {
        while (0 != OSActiveTasks)
        {
             mOSEnterCriticalSection();
                // extract the task with highest priority
                TempTaskWord = OSActiveTasks;
                TaskNo = 1;
                while (0 == (TempTaskWord & 0x01))
                {
                    TempTaskWord >>= 1;
                    TaskNo++;
                }
                // deactivate task
                OSActiveTasks &= (tOSTaskID)~(1<<(TaskNo-1));
            mOSLeaveCriticalSection();
            // execute
            pCallback = OSTaskCallbacks[TaskNo];
            if (NULL != pCallback)
            { pCallback();
            }
        }
        // OSIdleTask, should always be defined
        OSTaskCallbacks[0]();
    }
}
