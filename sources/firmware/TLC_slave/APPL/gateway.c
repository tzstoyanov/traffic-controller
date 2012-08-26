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
#include "os.h"

/*
 * local defines
 */

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
    tErrByte Error;
    U8 Status;
    U8 ErrorCnt;
    U8 Request;
    bool Timeout;
} tSlaveDevice;

typedef struct
{
    U16 WDCounter;
    U8 SlaveCount;
    U8 WDInterval;
    U8 DeviceId;
    U8 State;
    U8 DeadCount;
    U8 RespCnt;
    U8 RouterId;
    bool ResponseRequest;
} tGWData;

#define mCanBuildId(CMD, ADDR)  ((((U16)(CMD)) << 8)+(U8)(ADDR))
#define mCanExtractCmd(ID)      (((ID)>>8) & 0x07)
#define mCanExtractAddr(ID)     (U8)(ID)

/*
 * local variables
 */

static tGWData  GWData;
static tSlaveDevice Devices[cGWMaxDevices];

static U8 gw_find_slave_index(U8 ID)
{
    bool found = FALSE;
    U8   index;

    for (index=0; (index<GWData.SlaveCount)&&(!found); index++ )
    {
        if (Devices[index].Id == ID)
        {
            found = TRUE;
        }
    }

    return (found != FALSE) ? (index-1) : cGWMaxDevices;
}

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

        Result = (pMessage->Len == ((InputMsg.Len - 5)/2));

        { U8 ChkSum = 0;
            for (index=0;index<(pLINMsg->Len-2);index++)
            {
                ChkSum -= pLINMsg->Data[index];
            }
            Result = Result && (InputMsg.Data[InputMsg.Len-2] == (ChkSum>>4));
            Result = Result && (InputMsg.Data[InputMsg.Len-1] == (ChkSum & 0x0F));
        }


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

static void gw_code( tMessage * pMessage, tLINMsg * pLINMsg)
{
    static U8 H2A[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    U8 index;
    U8 part;

    index = 0;
    pLINMsg->Data[index] = H2A[pMessage->Command & 0x0F];
    index ++;
    pLINMsg->Data[index] = H2A[pMessage->Address >> 4];
    index ++;
    pLINMsg->Data[index] = H2A[pMessage->Address & 0x0F];
    index ++;
    pLINMsg->Data[index] = H2A[pMessage->Len & 0x0F];
    index ++;
    for (part = 0; part < (pMessage->Len & 0x0F); part++)
    {
        pLINMsg->Data[index] = H2A[pMessage->Data[part]>>4];
        index++;
        pLINMsg->Data[index] = H2A[pMessage->Data[part] & 0x0F];
        index++;
    }

    // checksum
    {
        U8 ChkSum = 0;
        U8 i;
        for (i=0; i<index; i++)
        {
            ChkSum -=pLINMsg->Data[i];
        }
        pLINMsg->Data[index] = H2A[ChkSum >> 4];
        index ++;
        pLINMsg->Data[index] = H2A[ChkSum & 0x0F];
        index ++;
    }
    pLINMsg->Len = index;
}

/**
 * \fn static gw_add_slave(U8 Id)
 *
 * add slave to be monitored
 *
 *
 * \param  [in] Id : U8 - slave id
 * \return none
 * \pre    component is initialized
 *
 * Uses    : GWData, Devices \n
 * Changes : GWdata, Devices
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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

/**
 * \fn static gw_remove_slave(U8 Id)
 *
 * remove device id from device buffer
 *
 * \param  [in] Id : U8 - slave id
 * \return none
 * \pre    component is initialized
 *
 * Uses    : <USES> \n
 * Changes : <CHANGES>
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static gw_remove_slave(U8 Id)
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
            Devices[index].Id       = 0;
            DEBUG_TRACE_U8("-- ", Id);
        }
    }
}

static gw_respond(void)
{
    tMessage CanMsg;
    tLINMsg  LinMsg;
    U8 index;

    if (FALSE != GWData.ResponseRequest)
    { // there is something to report
        index = 0;
        if (0 == GWData.RespCnt)
        { //insert GW state
            CanMsg.Data[0] = GWData.DeviceId;
            CanMsg.Data[1] = GWData.State;
            index +=2;
        }
        while ((index < 8) && (GWData.RespCnt < GWData.SlaveCount))
        { // add module state
            CanMsg.Data[index]   = Devices[GWData.RespCnt].Id;
            if ((TLC_TYPE_LAMP << 5) == Devices[GWData.RespCnt].Error.Data)
            { // lamp without error, report state
                CanMsg.Data[index+1] = (TLC_TYPE_LAMP << 5) | Devices[GWData.RespCnt].Status;
            }
            else
            { // report error
                CanMsg.Data[index+1] = Devices[GWData.RespCnt].Error.Data;
            }
            index +=2;
            GWData.RespCnt++;
        }
        CanMsg.Len = index;
        CanMsg.ID =
            mCanBuildId((GWData.RespCnt < GWData.SlaveCount) ? (TLC_CMD_WD_RESP_FRAG): (TLC_CMD_WD_RESP), GWData.RouterId);
        gw_code(&CanMsg, &LinMsg);
        lin_send(&LinMsg);
        GWData.ResponseRequest = (GWData.RespCnt < GWData.SlaveCount);
    }
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
                { // query interval, 10ms resolution
                    if ((index+1) < pMsg->Len)
                    {
                        GWData.WDInterval = pMsg->Data[index+1];
                        DEBUG_TRACE_U8("WD ", GWData.WDInterval)
                    }
                }
                else if (    (TLC_CFG_SUB_MODULES_ADD == param)
                      //    && (1 == length)
                         )
                { // add device
                    while ((length--) && ((index+1) < pMsg->Len))
                    {
                        gw_add_slave(pMsg->Data[index+1]);
                        index++;
                    }
                }
                else if (    (TLC_CFG_DEAD_COUNT == param)
                          && (1 == length)
                         )
                { // configure dead counter
                    GWData.DeadCount = pMsg->Data[index+1];
                }
                else if (    (TLC_CFG_COMM_TIMEOUT == param)
                          && (1 == length)
                         )
                { // configure communication timeout, 1ms resolution
                    // TODO
                }

                index +=length+1;
            }
            // TODO: call compact

            GWData.RespCnt = GWData.SlaveCount;
            GWData.State = 0x20;
            break;

        case TLC_CMD_WD_REQ:
            // state request
            if (1 == pMsg->Len)
            {
                GWData.RespCnt = 0;
                GWData.RouterId = pMsg->Data[0];
                GWData.ResponseRequest = TRUE;
            }
            break;

        default:
            DEBUG_TRACE("bad cmd");
            break;

    }
}

void appl_can_rx_handler(tCanMsg * pCanMsg)
{
    U16  index;

    if (   (mCanExtractCmd(pCanMsg->ID)  == TLC_CMD_WD_RESP)
        && (mCanExtractAddr(pCanMsg->ID) == GWData.DeviceId)
        && (pCanMsg->Length == 3)
        )
    { // state response received
        index = gw_find_slave_index(pCanMsg->Data[0]);
        if (cGWMaxDevices != index)
        {
            Devices[index].Timeout = FALSE;
            Devices[index].Error.Data   = pCanMsg->Data[1];
            Devices[index].Status  = pCanMsg->Data[2];
        }
    }
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

void appl_init(void)
{
    GWData.DeviceId = nvram_get_u8DeviceId();
    lin_init();
    can_init();
    OSSetPeriodicAlarm(cOS_TaskId_ApplTask, 2);

    GWData.DeadCount  = 3;
    GWData.WDInterval = 10;
    GWData.State      = 0x21;   // type:GW, state: ok, error: Init
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
    tMessage Msg;
    U16 index;
    bool found;

    if (NULL != pLINMsg)
    {
        if (gw_decode(pLINMsg, &Msg))
        {
            if (    (GWData.DeviceId == Msg.Address)
                 || (TLC_ADDR_ALL    == Msg.Address)
                 )
            { // message for the GW
                gw_command(&Msg);

                if (TLC_ADDR_ALL == Msg.Address)
                {
                    can_send(&Msg);
                }
            }
            else
            { // not for GW, send CAN frame
                if (cGWMaxDevices != gw_find_slave_index(Msg.Address))
                { // slave is registered
                    can_send(&Msg);
                }
            }
        }
    }
    else
    {
        Assert(FALSE,"null ptr");
    }
}

/**
 * \fn void gw_task(void)
 *
 * GW periodic task
 * called every 10ms
 *
 * Main purpose:
 * - management of long responses
 * - generation of CAN state requests
 * - dead counter
 *
 * \param  none
 * \return none
 * \pre    none
 *
 * Uses    : <USES> \n
 * Changes : <CHANGES>
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void appl_task(void)
{
    tCanMsg CanMsg;
    U16 index;

    /*
     * long responses
     */
    gw_respond();

    /*
     * Query management
     */
    GWData.WDCounter++;
    if (GWData.WDCounter >= (U16)GWData.WDInterval)
    { // query interval

        /*
         * check failure
         */
        for (index=0; index<GWData.SlaveCount; index++)
        {
            if (TRUE == Devices[index].Timeout)
            { // no response from device in this query interval
                if (Devices[index].ErrorCnt >= GWData.DeadCount)
                { // device is failed
                    Devices[index].Error.Error   = TLC_ERROR_TIMEOUT;
                    Devices[index].Error.ErrFlag = TRUE;
                }
                else
                { // increase error counter
                    Devices[index].ErrorCnt++;
                }
            }
        }

       /*
        * request new state
        */
        for (index=0; index<GWData.SlaveCount; index++)
        {
            Devices[index].Timeout = TRUE;
        }

        CanMsg.ID = mCanBuildId(TLC_CMD_WD_REQ, TLC_ADDR_ALL);
        CanMsg.Length = 1;
        CanMsg.Data[0] = GWData.DeviceId;
        can_send(&CanMsg);

        /*
         * start new interval
         */
        GWData.WDCounter = 0;
    }
}

#endif
/**    @}
 * @}
 */
