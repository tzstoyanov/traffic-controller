/**
 * \file
 * \project TLC
 * \brief   simple CAN driver (cfg)
 *
 * \details
 * The file implements a basic CAN RX and TX queue
 *
 * \author  taa
 * \date    16.08.2012
 *
 * \addtogroup Drivers
 * \{
 *      \addtogroup CAN
 *      \{
 */

/* change log:
 **
 * 16.08.2012 taa: creation
 *
 *//* ************************************************************************ */

#ifndef CAN_CFG_H
#define CAN_CFG_H


// CANCLKDIV => CAN frequency = 8MHz
// CAN_BTR: 8TQ, TSEG1 = 5, TSEG2 = 2, SP = 62%, 1MHz/8=125k
#define cCanInitTableInitializer    { 0x00000005UL, 0x000014C7UL }

#define cCANQueueSize       32
    /**< size of TX and RX queues */

#define cCANTxMsgBox        31
    /**< id of the message box, used for transmission */

extern void appl_can_rx_handler(tCanMsg * pCanMsg);
#define cCANRxCallback  appl_can_rx_handler
    /**< function to be called upon CAN frame  reception */

#define cCANTxTimeoutInMs    5000
    /**< TX timeout */

#define cCANTickinMs         10
    /**< can task calling interval */

#endif

/** \}\} */
/*** End of file **************************************************************/
