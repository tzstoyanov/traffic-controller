/**
 * \file
 * \project TLS
 * \brief   system configuration file
 *
 * \details
 * system wide definitions for project TLC
 * Board Olimex LPC-P11C24
 *
 * \author  taa
 * \date    15.08.2012
 *
 *
 * \addtogroup System
 * \{ System components
 *      \addtogroup Configuration
 *      \{
 */

/* change log:
 **
 * 15.08.2012 taa: creation
 * 20.08.2012 taa: ++ UART_TRACE_DEFINE
 *
 *//* ************************************************************************ */

#ifndef SYSTEM_H
 #define SYSTEM_H

#define DEBUG_ASSERT
#define SILENT_ASSERT

/*
 * device definitions
 */
#define DEVICE_TYPE_GATEWAY     0
#define DEVICE_TYPE_LAMP        1
#define DEVICE_TYPE_TIMER       2

#define DEVICE_TYPE     DEVICE_TYPE_GATEWAY
    /**< device type */



#define UART_TRACE_CAN
    /**< uncomment to enable CAN trace over UART */
//#define CAN_TEST
    /**< uncomment to put CAN cell in test mode */

#endif

/** \}\} */
/*** End of file **************************************************************/

