/**
 * @file  lamp.c
 * @brief slave lamp controller (code)
 * @addtogroup Application
 * @{
 *     @addtogroup Lamp
 *     @{
 */

#include "system.h"

#if DEVICE_TYPE == DEVICE_TYPE_LAMP

#include "types.h"
#include "can.h"
#include "tlc_cfg.h"
#include "assert.h"
#include "lpc11xx_gpio.h"
#include "nvram.h"

/*
 * local types
 */
typedef struct
{
    U8 DeviceId;    /**< device identifier */
} tLampData;

/*
 * local variables
 */
static tLampData LampData;

/**
 * \fn void lamp_init(void)
 *
 * initialization routine
 *
 *
 * \param  none
 * \return none
 * \pre    none
 *
 * Uses    : none \n
 * Changes : <CHANGES>
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void lamp_init(void)
{
    LampData.DeviceId = nvram_get_u8DeviceId();
    can_init();
}

/**
 * \fn void appl_can_rx_handler(tCanMsg * pCanMsg)
 *
 * CAN command dispatcher
 *
 * called upon CAN message reception
 *
 * \param  [in] pCanMsg : tCanMsg* - received message
 * \return none
 * \pre    <PRECONDITIONS>
 *
 * Uses    : <USES> \n
 * Changes : <CHANGES>
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void appl_can_rx_handler(tCanMsg * pCanMsg)
{
    if (   (((pCanMsg->ID & 0x700)>>8) == TLC_CMD_STATE)
        && ((pCanMsg->ID & 0x0FF)      == LampData.DeviceId)
        && (pCanMsg->Length == 1)
        )
    {
        if (((pCanMsg->Data[0] & 0xE0)>>5) == TLC_TYPE_LAMP)
        {
            if ((pCanMsg->Data[0] & 0x01) == 0)
            {
                GPIO_SetBits(PORT1, GPIO_Pin_9);
            }
            else
            {
                GPIO_ResetBits(PORT1, GPIO_Pin_9);
            }
        }
    }
}

#endif
/**    @}
 * @}
 */
