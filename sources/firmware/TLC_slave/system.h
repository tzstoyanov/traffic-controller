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

#endif

/** \}\} */
/*** End of file **************************************************************/

