/**
 * @file  gateway.c
 * @brief CAN<>LIN gateway (code)
 * @addtogroup Application
 * @{
 *     @addtogroup Gateway
 *     @{
 */


#include "system.h"


#if DEVICE_TYPE == DEVICE_TYPE_GATEWAY
#include "types.h"
#include "lin.h"
#include "can.h"
#include "tlc_cfg.h"
#include "assert.h"
#include "lpc11xx_gpio.h"
#include "nvram.h"

/*
 * local defines
 */

#define UART_TRACE_GW
    /**< uncomment to trace GW */

#define cGWMaxDevices       32


#ifdef UART_TRACE_GW
    #define DEBUG_TRACE(TRACE)      {TSTOutStr("\n\rGW:"); TSTOutStr(TRACE);}
    #define DEBUG_TRACE_U8(TRACE,VAL)   {TSTOutStr("\n\rGW:"); TSTOutStr(TRACE); TSTOut8H(VAL);}
#else
    #define DEBUG_TRACE(TRACE)
    #define DEBUG_TRACE_U8(TRACE,VAL)
#endif

/*
 * local types
 */
typedef struct
{
    union
    {
        struct
        {
            U8  Address;
            U8  Command;
        };
        U16 ID;
    };

    U8  Data[8];
    U8  Len;
} tMessage;

typedef struct
{
    U8 Id;
    U8 Status;
    U8 ErrorCnt;
    bool Timeout;
} tSlaveDevice;

typedef struct
{
    U8 DeviceId;
    U8 WDCounter;
    U8 SlaveCount;
    U8 WDInterval;
} tGWData;

/*
 * local variables
 */

static tGWData  GWData;
static tSlaveDevice Devices[cGWMaxDevices];

static U8 ascii2hex(U8 data)
{
    U8 Result = 16;

    if ((data>='0') && (data<='9'))
    {
        Result = data - '0';
    }
    else if ((data>='a') && (data<='f'))
    {
        Result = 10+ data - 'a';
    }
    else if ((data>='A') && (data<='F'))
    {
        Result = 10+ data - 'A';
    }
    else
    { // error
    }
    return Result;
}

static bool gw_decode(tLINMsg * pLINMsg, tMessage * pMessage)
{
    U8 index;
    U8 data = 0;
    tLINMsg InputMsg;

    bool second = TRUE;
    bool Result = TRUE;

    InputMsg.Len = pLINMsg->Len;
    for (index=0; index < pLINMsg->Len; index++)
    {
        InputMsg.Data[index] = ascii2hex(pLINMsg->Data[index]);

        if (InputMsg.Data[index] >= 16)
        {
            Result = FALSE;
            break;
        }
    }

    Result = Result && (0 == (InputMsg.Len % 2));

    if (Result)
    {
        pMessage->Command = InputMsg.Data[0];
        pMessage->Address = (InputMsg.Data[1]<<4) + InputMsg.Data[2];
        pMessage->Len     = InputMsg.Data[3];

        Result = (pMessage->Len == ((InputMsg.Len - 4)/2));

        if (Result)
        {
            for (index = 0; index < pMessage->Len; index++)
            {
                pMessage->Data[index] = (InputMsg.Data[4+(2*index)] << 4)+ InputMsg.Data[4+(2*index)+1];
            }
        }
    }
    return Result;
}

static gw_add_slave(U8 Id)
{
    U16 index;
    bool found = FALSE;

    if (TLC_ADDR_INVALID == Id)
    { //remove all devices
        GWData.SlaveCount = 0;
        DEBUG_TRACE("-- all");
    }
    else
    {
        for (index = 0; (index<GWData.SlaveCount) && !found; index++)
        {
            if (Id == Devices[index].Id)
            {
                found = TRUE;
            }
        }

        if ((!found) && (GWData.SlaveCount < cGWMaxDevices))
        {
            Devices[index].Id       = Id;
            Devices[index].ErrorCnt = 0;
            Devices[index].Status   = TLC_ERROR_INIT;
            Devices[index].Timeout  = FALSE;
            GWData.SlaveCount++;
            DEBUG_TRACE_U8("++ ", Id);
        }
    }
}

static gw_remove_slave(U8 Id)
{
}

/**
 * \fn void gw_command(tMessage * pMsg)
 *
 * dispatch GW commands
 *
 *
 * \param  [in] pMsg : tMessage - message to the gateway
 * \return none
 * \pre    component is initialized
 *
 * Uses    : <USES> \n
 * Changes : <CHANGES>
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void gw_command(tMessage * pMsg)
{
    U8 index = 0;
    U8 state = 0;


    switch (pMsg->Command)
    {
        case TLC_CMD_CONFIG:
            // frame contains configuration parameters
            index = 0;
            while (index < pMsg->Len)
            {
                U8 param    = pMsg->Data[index] >> 4;
                U8 length   = pMsg->Data[index] & 0x0F;

                if (   (TLC_CFG_QUERY_INTERVAL == param)
                    && (1 == length)
                    )
                { // query interval
                    if ((index+1) < pMsg->Len)
                    {
                        GWData.WDInterval = pMsg->Data[index+1];
                        DEBUG_TRACE_U8("WD ", GWData.WDInterval)
                    }
                }
                else if (    (TLC_CFG_SUB_MODULES_ADD == param)
                          && (1 == length)
                         )
                { // add device
                    if ((index+1) < pMsg->Len)
                    {
                        gw_add_slave(pMsg->Data[index+1]);
                    }
                }
                else if (    (TLC_CFG_SUB_MODULES_REMOVE == param)
                          && (1 == length)
                         )
                { // add device
                    if ((index+1) < pMsg->Len)
                    {
                        gw_remove_slave(pMsg->Data[index+1]);
                    }
                }
                index +=length+1;
            }
            break;

        case TLC_CMD_WD_REQ:
            // state request

            break;

        default:
            DEBUG_TRACE("bad cmd");
            break;

    }
}

void appl_can_rx_handler(tCanMsg * pCanMsg)
{
//    if (   (((pCanMsg->ID & 0x700)>>8) == TLC_CMD_STATE)
//        && ((pCanMsg->ID & 0x0FF)      == TLC_ID)
//        && (pCanMsg->Length == 1)
//        )
//    {
//        if (((pCanMsg->Data[0] & 0xE0)>>5) == TLC_TYPE_LAMP)
//        {
//            if ((pCanMsg->Data[0] & 0x01) == 0)
//            {
//                GPIO_SetBits(PORT1, GPIO_Pin_9);
//            }
//            else
//            {
//                GPIO_ResetBits(PORT1, GPIO_Pin_9);
//            }
//        }
//    }
}

/**
 * \fn void gw_init(void)
 *
 * gateway initialization
 *
 * \param  none
 * \return none
 * \pre    none
 *
 * Uses    : NVRAM \n
 * Changes : GWData
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void gw_init(void)
{
    GWData.DeviceId = nvram_get_u8DeviceId();
    lin_init();
    can_init();
}

/**
 * \fn void gw_lin_reception(tLINMsg * pLINMsg)
 *
 * LIN message is recevied
 *
 * \param  [in] pLINMsg : tLINMsg * - message
 * \return none
 * \pre    component is initilized
 *
 * Uses    : <USES> \n
 * Changes : <CHANGES>
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void gw_lin_reception(tLINMsg * pLINMsg)
{
    static tMessage Msg;

    if (NULL != pLINMsg)
    {
        if (gw_decode(pLINMsg, &Msg))
        {
            if (GWData.DeviceId == Msg.Address)
            { // message for the GW
                gw_command(&Msg);
            }
            else
            { // send CAN frame
                can_send(&Msg);
            }
        }
    }
    else
    {
        Assert(FALSE,"null ptr");
    }
}

#endif
/**    @}
 * @}
 */
