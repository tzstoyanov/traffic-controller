/**
 * \file
 * \project TLC
 * \brief   CAN driver (interface)
 *
 * \details
 * Definitions for CAN driver
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

#ifndef CAN_H
#define CAN_H

typedef struct
{
    U16 ID;
    U8  Data[8];
    U8  Length;
} tCanMsg;

typedef void (*tpCANRxCallback) (tCanMsg * pCanMsg);

extern void can_init   (void);
extern void can_deinit (void);
extern void can_tx_task(void);
extern void can_rx_task(void);

extern bool can_send(tCanMsg * pCanMsg);

#endif
/** \}\} */
/*** End of file **************************************************************/
