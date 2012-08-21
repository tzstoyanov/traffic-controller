/**
 * \file
 * \project TLC
 * \brief   OS configuration file
 *
 *
 * \author  taa
 * \date    15.08.2012
 *
 * \addtogroup System
 * \{ System components
 *      \addtogroup OS
 *      \{
 */

/* change log:
 **
 * 15.08.2012 taa: creation
 * 20.08.2012 taa: ++ GW/lamp task
 *
 *//* ************************************************************************ */

#ifndef OS_CFG_H
#define OS_CFG_H

#include "system.h"

/*
 * System clock
 */
    #define cOsTimeInMicroSeconds   1250
        /**< OS base tick time in us */

    #define cOSTickInTicks  \
        (((SystemFrequency /1000)* cOsTimeInMicroSeconds)/1000)
        /**< OS base tick time in cpu ticks */

/*
 * Task IDs
 * should correspond to the task order into mOSTaskCallbacksInitilizer
 */

    #define cOS_TaskId_CanTask      ((tOSTaskID)(1<<0))
    #define cOS_TaskId_CanTxTask    ((tOSTaskID)(1<<1))
    #define cOS_TaskId_CanRxTask    ((tOSTaskID)(1<<2))
    #define cOS_TaskId_LinTask      ((tOSTaskID)(1<<3))
    #define cOS_TaskId_LinRxTask    ((tOSTaskID)(1<<4))
    #define cOS_TaskId_ApplTask     ((tOSTaskID)(1<<5))
    #define cOS_TaskId_TimerTask    ((tOSTaskID)(1<<6))

/*
 * Tasks
 * external function definitions
 */
    extern void IdleTask(void);
    extern void can_task(void);
    extern void can_tx_task(void);
    extern void can_rx_task(void);
    extern void lin_task(void);
    extern void lin_rx_task(void);
    extern void OSTimerTask(void);
    extern void appl_task(void);
/*
 * Task list
 * The initializer of OS task list, smaller index are with greater priority, except index 0 which is IDLE task
 */
    #define mOSTaskCallbacksInitilizer()    \
    { IdleTask,                             \
      can_task,                             \
      can_tx_task,                          \
      can_rx_task,                          \
      lin_task,                             \
      lin_rx_task,                          \
      appl_task,                            \
      OSTimerTask,                          \
      NULL,                                 \
      NULL,                                 \
      NULL,                                 \
      NULL,                                 \
      NULL,                                 \
      NULL,                                 \
      NULL,                                 \
    }

#endif

/** \}\} */
/*** End of file **************************************************************/
