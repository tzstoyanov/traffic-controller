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
    tTLCErrByte ErrByte;
} tLampData;


#define LAMP_INSTANCES  3
#define LAMPS_ID    {0x03, 0x04, 0x05}

static const u08    cLampIds[]  =   LAMPS_ID;
#define LAMPS_ID_COUNT      ((sizeof cLampIds)/(sizeof cLampIds[0]))

/*
 * local variables
 */
static tLampData LampData[LAMPS_ID_COUNT];

void appl_can_rx_handler(u08 instance, tCanMsg * pCanMsg);

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
    REG i;
    can_init();

    for (i=0; i<LAMPS_ID_COUNT; i++)
    {
        //LampData[i].DeviceId = nvram_get_u8DeviceId();
        LampData[i].DeviceId = cLampIds[i];
        LampData[i].ErrByte.Error = TLC_ERROR_INIT;
        LampData[i].ErrByte.ErrFlag = TRUE;
        LampData[i].ErrByte.Type  = TLC_TYPE_LAMP;
        // register callback
        can_register_cb(cLampIds[i], &appl_can_rx_handler, i);
    }
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

void appl_can_rx_handler(u08 instance, tCanMsg * pCanMsg)
{
    tCanMsg CanMsg;
    tLampData * pLampData;

    if (instance < LAMP_INSTANCES)
    {
        pLampData = &LampData[instance];

        if (   (((pCanMsg->ID & 0x700)>>8) == TLC_CMD_STATE)
            && (pCanMsg->Length == 1)
            )
        {
            if (((pCanMsg->Data[0] & 0xE0)>>5) == TLC_TYPE_LAMP)
            {
                if (0 == instance)
                {
                    mDio_SetPin(GREEN , (0 != (pCanMsg->Data[0] & 0x01)));
                    mDio_SetPin(YELLOW, (0 != (pCanMsg->Data[0] & 0x02)));
                    mDio_SetPin(RED   , (0 != (pCanMsg->Data[0] & 0x04)));
                }
                // TODO: configure all instances

                pLampData->Status = pCanMsg->Data[0] & 0x07;

                if (TLC_ERROR_INIT == pLampData->ErrByte.Error)
                {
                    pLampData->ErrByte.Error = TLC_ERROR_OK;
                    pLampData->ErrByte.ErrFlag = FALSE;
                }
            }
        }
        else if (   (((pCanMsg->ID & 0x700)>>8) == TLC_CMD_WD_REQ)
                && (pCanMsg->Length == 1)
                )
        {
            CanMsg.Data[0] = pLampData->DeviceId;
            CanMsg.Data[1] = pLampData->ErrByte.Data;
            CanMsg.Data[2] = pLampData->Status;
            CanMsg.Length = 3;
            CanMsg.ID = pCanMsg->Data[0] + (((U16)(TLC_CMD_WD_RESP))<<8);
            can_send(&CanMsg);
        }
    }
    else
    {
        Assert(FALSE, "out bounds");
    }
}


void appl_task(void)
{

}
#endif
/**    @}
 * @}
 */
