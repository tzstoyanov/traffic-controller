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
#include "nvram.h"
#include "dio.h"
#include "os.h"

/*
 * local types
 */

typedef struct
{
    u08  DeviceId;    /**< device identifier */
    u08  Status;
    tTLCErrByte ErrByte;
    u08  DeadCount;
    u08  WDInterval;
    u08  ErrorCnt;
    u08  WDCounter;
    bool Timeout;

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
static void lamp_goto_comm_err(tLampData * pLampData);
static void lamp_goto_ok(tLampData * pLampData);


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
        LampData[i].DeviceId        = cLampIds[i];
        LampData[i].ErrByte.Error   = TLC_ERROR_INIT;
        LampData[i].ErrByte.ErrFlag = TRUE;
        LampData[i].ErrByte.Type    = TLC_TYPE_LAMP;
        LampData[i].DeadCount  = 3;
        LampData[i].ErrorCnt   = 0;
        LampData[i].Status     = 0;
        LampData[i].Timeout    = FALSE;
        LampData[i].WDCounter  = 0;
        LampData[i].WDInterval = 10;
        // register callback
        can_register_cb(cLampIds[i], &appl_can_rx_handler, i);
    }
    OSSetPeriodicAlarm(cOS_TaskId_ApplTask, 2);
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
    bool ValidCmd = FALSE;

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
//                    GPIO_SetDir(PORT1, GPIO_Pin_9 , (0 != (pCanMsg->Data[0] & 0x01)));
//                    GPIO_SetDir(PORT1, GPIO_Pin_11, (0 != (pCanMsg->Data[0] & 0x02)));
//                    GPIO_SetDir(PORT1, GPIO_Pin_10, (0 != (pCanMsg->Data[0] & 0x04)));

                    mDio_SetPin(GREEN , (0 != (pCanMsg->Data[0] & 0x01)));
                    mDio_SetPin(YELLOW, (0 != (pCanMsg->Data[0] & 0x02)));
                    mDio_SetPin(RED   , (0 != (pCanMsg->Data[0] & 0x04)));
                }
                // TODO: configure all instances

                pLampData->Status = pCanMsg->Data[0] & 0x07;
            }
            if (TLC_ERROR_INIT == pLampData->ErrByte.Error)
            {
                lamp_goto_ok(pLampData);
            }
            ValidCmd = TRUE;
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

            ValidCmd = TRUE;
        }
        else if (((pCanMsg->ID & 0x700)>>8) == TLC_CMD_CONFIG)
        {
            // frame contains configuration parameters
            u08 index = 0;
            while (index < pCanMsg->Length)
            {
                U8 param    = pCanMsg->Data[index] >> 4;
                U8 length   = pCanMsg->Data[index] & 0x0F;

                if (   (TLC_CFG_QUERY_INTERVAL == param)
                    && (1 == length)
                    )
                { // query interval, 10ms resolution
                    if ((index+1) < pCanMsg->Length)
                    {
                        pLampData->WDInterval = pCanMsg->Data[index+1];
                    }
                }
                else if (    (TLC_CFG_DEAD_COUNT == param)
                          && (1 == length)
                         )
                { // configure dead counter
                    pLampData->DeadCount = pCanMsg->Data[index+1];
                }
                else
                { // skip
                }
                index +=length+1;
            }
            ValidCmd = TRUE;
        }

        if (FALSE != ValidCmd)
        { // leave comm error state
            pLampData->Timeout = FALSE;
            if (TLC_ERROR_COMM == pLampData->ErrByte.Error)
            {
                lamp_goto_ok(pLampData);
            }
        }
    }
    else
    {
        Assert(FALSE, "out bounds");
    }
}

/**
 * \fn void appl_task(void)
 *
 * periodic task
 *
 * \param  none
 * \return none
 * \pre    <PRECONDITIONS>
 *
 * Uses    : <USES> \n
 * Changes : <CHANGES>
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void appl_task(void)
{
    u08 i;
    tLampData * pLampData;

    for (i = 0; i< LAMP_INSTANCES; i++)
    {
        pLampData = &LampData[i];
        if (TLC_ERROR_INIT == pLampData->ErrByte.Error)
        {

        }
        else if (TLC_ERROR_COMM == pLampData->ErrByte.Error)
        {

        }
        else
        {
            pLampData->WDCounter++;
            if (pLampData->WDCounter >= (U16)pLampData->WDInterval)
            {
                if (TRUE == pLampData->Timeout)
                { // no response from device in this query interval
                    if (pLampData->ErrorCnt >= pLampData->DeadCount)
                    { // device is failed
                        lamp_goto_comm_err(pLampData);
                    }
                    else
                    { // increase error counter
                        pLampData->ErrorCnt++;
                    }
                }
                pLampData->Timeout = TRUE;
                pLampData->WDCounter = 0;
            }
        }
    }
}


static void lamp_goto_ok(tLampData * pLampData)
{
    pLampData->ErrByte.ErrFlag = FALSE;
    pLampData->ErrByte.Error   = TLC_ERROR_OK;
    pLampData->ErrorCnt  = 0;
    pLampData->WDCounter = 0;
    pLampData->Timeout   = FALSE;
}

static void lamp_goto_comm_err(tLampData * pLampData)
{
    pLampData->ErrByte.ErrFlag = TRUE;
    pLampData->ErrByte.Error   = TLC_ERROR_COMM;
    pLampData->Status  = 0;
    // todo: insert code to shut down outputs
    if (cLampIds[0] == pLampData->DeviceId)
    {
        mDio_SetPin(GREEN , 0);
        mDio_SetPin(YELLOW, 0);
        mDio_SetPin(RED   , 0);
    }
}

#endif
/**    @}
 * @}
 */
