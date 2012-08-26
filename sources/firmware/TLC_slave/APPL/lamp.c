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
#include "dio.h"

/*
 * local types
 */

typedef struct
{
    U8 DeviceId;    /**< device identifier */
    U8 Status;
    tErrByte ErrByte;
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

void appl_init(void)
{
    LampData.DeviceId = nvram_get_u8DeviceId();
    LampData.ErrByte.Error = TLC_ERROR_INIT;
    LampData.ErrByte.ErrFlag = TRUE;
    LampData.ErrByte.Type  = TLC_TYPE_LAMP;
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
    tCanMsg CanMsg;
    U8 Temp;

    if (   ((pCanMsg->ID & 0x0FF)== LampData.DeviceId)
        || ((pCanMsg->ID & 0x0FF)== TLC_ADDR_ALL)
        )
    {
        if (   (((pCanMsg->ID & 0x700)>>8) == TLC_CMD_STATE)
            && (pCanMsg->Length == 1)
            )
        {
            if (((pCanMsg->Data[0] & 0xE0)>>5) == TLC_TYPE_LAMP)
            {
                mDio_SetPin(GREEN , (0 != (pCanMsg->Data[0] & 0x01)));
                mDio_SetPin(RED   , (0 != (pCanMsg->Data[0] & 0x02)));
                mDio_SetPin(YELLOW, (0 != (pCanMsg->Data[0] & 0x04)));

                LampData.Status = pCanMsg->Data[0] & 0x07;

                if (TLC_ERROR_INIT == LampData.ErrByte.Error)
                {
                    LampData.ErrByte.Error = TLC_ERROR_OK;
                    LampData.ErrByte.ErrFlag = FALSE;
                }
            }
        }
        else if (   (((pCanMsg->ID & 0x700)>>8) == TLC_CMD_WD_REQ)
                && (pCanMsg->Length == 1)
                )
        {
            CanMsg.Data[0] = LampData.DeviceId;
            CanMsg.Data[1] = LampData.ErrByte.Data;
            CanMsg.Data[2] = LampData.Status;
            CanMsg.Length = 3;
            CanMsg.ID = pCanMsg->Data[0] + (((U16)(TLC_CMD_WD_RESP))<<8);
            can_send(&CanMsg);
        }
    }
}


void appl_task(void)
{

}
#endif
/**    @}
 * @}
 */
