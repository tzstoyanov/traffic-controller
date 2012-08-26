/**
 * @file  lin.c
 * @brief LIN driver (cfg)
 * @addtogroup Drivers
 * @{
 *     @addtogroup LIN
 *     @{
 */
#ifndef LIN_CFG_H
#define LIN_CFG_H

#include "tlc_cfg.h"

#define cLINBaudRate    115200
    /**< select UART baudrate */

#define cLINCmdStartChar    TL_PKT_START
#define cLINCmdEndChar      TL_PKT_END

extern void gw_lin_reception(tLINMsg * LINMsg);
#define LIN_RX_CALLBACK_PTR    gw_lin_reception

//#define NO_TX_REPLY

#endif
/**    @}
 * @}
 */
