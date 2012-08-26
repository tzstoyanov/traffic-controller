/**
 * @file  lin.c
 * @brief LIN driver (code)
 * @addtogroup Drivers
 * @{
 *     @addtogroup LIN
 *     @{
 */
 
/*
 * include files
 */
#include "types.h"
#include "lin.h"        // provided interface
#include "lin_cfg.h"    // configuration
#include "os.h"
#include "system.h"

#include "lpc11xx_uart.h"
#include "tst.h"
#include "assert.h"

/*
 * local defines
 */


// UART RX states
#define cLINCmdState_WaitCmdStart   0
#define cLINCmdState_WaitCmdEnd     1

// UART TC states
#define cLINRspState_Idle       0
#define cLINRspState_PreSend    1
#define cLINRspState_Send       2
#define cLINRspState_PostSend   3
#define cLINRspState_Check      4


#define mConvHexDigitsToByte(A,B)   (((A)<<4) + (B))
#define mLoTetrade(A)   (U8)((A)&0x0F)
#define mHiTetrade(A)   (U8)((A)>>4)

typedef struct
{
    U8 Data[cLINCmdMaxLength];
    U8 Len;
} tLINMsg;

typedef struct
{
    U8 CmdBuffer[cLINCmdMaxLength];
    U8 RspBuffer[cLINCmdMaxLength];
    tpLINRxCallBack pRxCallback;

    U8 CmdState;
    U8 CmdLength;
    U8 RspState;
    U8 RspLength;
    U8 RspCnt;

} tLINData;

/*
 * local variables
 */

static tLINData LINData;
static tLINMsg  LINRxMsg;

/*
 * local function prototypes
 */

    static void lin_rx_callback(void);
    static void lin_collect(void);
    static void lin_transmit(void);

    static U8 checksum(U8 * pData, U8 Length);
    static U8 ascii2hex(U8 data);
    static bool lin_decode(tLINMsg * pLINMsg, tMessage * pMessage);
    static void lin_code( tMessage * pMessage, tLINMsg * pLINMsg);


/*
 * local function implementation
 */

/** @fn void lin_collect(void)
 * collect data from UART FIFO
 *
 * @param  none
 * @return none
 * @pre    component is initialized
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static void lin_collect(void)
{
    U8 Data;

    while(UART_GetLineStatus(LPC_UART) & UART_LSR_RDR)
    { // data are available
        Data = UART_ReceiveData(LPC_UART);

        if (cLINCmdState_WaitCmdStart == LINData.CmdState)
        {
            if (cLINCmdStartChar == Data)
            { // cmd char received
                LINData.CmdState = cLINCmdState_WaitCmdEnd;
            }
            else
            { // error in communication
                // TODO: increase error counter
            }
        }
        else if (cLINCmdState_WaitCmdEnd == LINData.CmdState)
        {
            if (cLINCmdEndChar == Data)
            { // cmd end char received
#ifndef NO_TX_REPLY
                if (cLINRspState_Check == LINData.RspState)
                { // compare with RspBuffer
                    // TODO: compare
                    LINData.RspState = cLINRspState_Idle;
                }
                else
#else
                LINData.RspState = cLINRspState_Idle;
#endif
                {
                    // TODO: resolve case with pending message ?!?
                    U8 index;
                    for (index = 0; index < LINData.CmdLength; index++)
                    {
                        LINRxMsg.Data[index] = LINData.CmdBuffer[index];
                    }
                    LINRxMsg.Len = LINData.CmdLength;
                    OSActivateTask(cOS_TaskId_LinRxTask);
                }

                LINData.CmdLength = 0;
                LINData.CmdState = cLINCmdState_WaitCmdStart;
            }
            else if (cLINCmdMaxLength > LINData.CmdLength)
            { // collect data into cmd buffer
                LINData.CmdBuffer[LINData.CmdLength] = Data;
                LINData.CmdLength++;
            }
            else
            { // error in communication
                LINData.CmdLength = 0;
                LINData.CmdState = cLINCmdState_WaitCmdStart;
                // TODO: increase bad command counter
            }
        }
        else
        { // error: invalid state
            Assert(FALSE,"state");
        }
    }
}

/** @fn static void lin_transmit(void)
 * collect data from UART FIFO
 *
 * @param  none
 * @return none
 * @pre    component is initialized
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static void lin_transmit(void)
{
    while(UART_GetLineStatus(LPC_UART) & UART_LSR_THRE)
    { // data can be sent
        switch (LINData.RspState)
        {
        case cLINRspState_PreSend:
            UART_SendData(LPC_UART, cLINCmdStartChar);
            LINData.RspState = (LINData.RspLength>0) ? cLINRspState_Send : cLINRspState_PostSend;
            break;
        case cLINRspState_Send:
            if (cLINCmdMaxLength > LINData.RspCnt)
            {
                UART_SendData(LPC_UART, LINData.RspBuffer[LINData.RspCnt]);
                LINData.RspCnt++;
                if (LINData.RspCnt >= LINData.RspLength)
                {
                    LINData.RspState = cLINRspState_PostSend;
                }
            }
            break;
        case cLINRspState_PostSend:
            UART_SendData(LPC_UART, cLINCmdEndChar);
            LINData.RspState = cLINRspState_Check;
            break;
        default:
            break;
        }
    }
}

/*
 * exported functions
 */

/** @fn void lin_init(void)
 * initialization routine
 *
 * @param  none
 * @return none
 * @pre none
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void lin_init(void)
{
    UART_CFG_Type
        UART_CFG = { cLINBaudRate, UART_PARITY_NONE, UART_DATABIT_8, UART_STOPBIT_1};
    UART_FIFO_CFG_Type
        UART_FIFO_CFG = {ENABLE, ENABLE, UART_FIFO_TRGLEV3};

    LINData.CmdState    = cLINCmdState_WaitCmdStart;
    LINData.RspState    = cLINRspState_Idle;
    LINData.pRxCallback = LIN_RX_CALLBACK_PTR;

    UART_PinsInit();
    UART_Init(LPC_UART, &UART_CFG);
    UART_FIFOConfig(LPC_UART, &UART_FIFO_CFG);

    UART_SetupCbs (LPC_UART, 0, &lin_rx_callback);
    UART_IntConfig(LPC_UART, UART_INTCFG_RBR, ENABLE);
    UART_TxCmd    (LPC_UART, ENABLE);

    //NVIC_EnableIRQ(UART_IRQn);

#ifdef UART_TRACE_LIN
    TSTOutStr("\r\nLIN: init");
#endif
    // start lin task
    //OSSetPeriodicAlarm(cOS_TaskId_LinTask,0);
}

/** @fn void lin_task(void)
 * periodic task
 *
 * @param  none
 * @return none
 * @pre component is initialized
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void lin_task(void)
{
    UART_IntConfig(LPC_UART, UART_INTCFG_RBR, DISABLE);
    lin_collect();
    UART_IntConfig(LPC_UART, UART_INTCFG_RBR, ENABLE);

    if (    (cLINRspState_Idle  != LINData.RspState)
         && (cLINRspState_Check != LINData.RspState)
         )
    {
        lin_transmit();
    }
}
 

/** @fn void lin_rx_task(void)
 * reception task, non periodic
 *
 * @param  none
 * @return none
 * @pre component is initialized
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void lin_rx_task(void)
{
    tMessage Message;
    if (FALSE != lin_decode(&LINRxMsg, &Message))
    {
        if (NULL != LINData.pRxCallback)
        {
            LINData.pRxCallback(&Message);
        }
    }

    LINRxMsg.Len = 0;
}

/** @fn void lin_send(U8 * pData, U8 Len)
 * send response
 *
 * @param  [in] pData: U8 * - pointer to data
 * @param  [in] Len  : U8   - data length
 * @return none
 * @pre component is initialized
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void lin_send(tLINMsg * pLINMsg)
{
    U8 index;

    if (pLINMsg != NULL)
    {
        if (cLINRspState_Idle == LINData.RspState)
        { // driver is able to accept commands
            if (pLINMsg->Len < cLINCmdMaxLength)
            {
                for (index = 0; index < pLINMsg->Len; index++)
                {
                    LINData.RspBuffer[index] = pLINMsg->Data[index];
                }
                LINData.RspLength = pLINMsg->Len;
                LINData.RspCnt    = 0;
                LINData.RspState = cLINRspState_PreSend;
                lin_transmit();
            }
            else
            { // error
                Assert(FALSE,"out bounds");
            }
        }
        else
        { // error ?!?
            Assert(FALSE,"not ready");
        }
    }
    else
    {
        Assert(FALSE,"null ptr");
    }
}


void lin_send_message(tMessage * pMessage)
{
    tLINMsg LinMsg;
    lin_code(pMessage, &LinMsg);
    lin_send(&LinMsg);
}

/** @fn void lin_rx_callback(void)
 * periodic task
 *
 * @param  none
 * @return none
 * @pre component is initialized
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static void lin_rx_callback(void)
{
    lin_collect();
}

/**
 * \fn static U8 checksum(U8 * pData, U8 Length)
 *
 * calculates checksum = (U8)(0 - sum(pData[<i>])), where <i> = 0,1,..,Length
 *
 * \param  [in] pData  : U8 * - pointer to data
 * \param  [in] Length : U8   - length of the data buffer
 * \return U8 - checksum
 * \pre    none
 *
 * Uses    : none \n
 * Changes : none
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static U8 checksum(U8 * pData, U8 Length)
{
    U8 Result = 0;
    U32 index;

    for (index=0; index<Length; index++)
    {
        Result -= pData[index];
    }

    return Result;
}

/**
 * \fn static U8 ascii2hex(U8 data)
 *
 * converts ASCII character to hex (0-15)
 *
 * \param  [in] data : U8 - ASCII character [0-9,'a'-'f','A'-'F']
 * \return U8 - hex digit 0-15
 * \pre    none
 *
 * Uses    : none \n
 * Changes : none
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
    { // error in input
    }
    return Result;
}

/**
 * \fn static bool lin_decode(tLINMsg * pLINMsg, tMessage * pMessage)
 *
 * decode LIN frame
 *
 * <DESCRIPTION>
 *
 * \param  [in]  pLINMsg  : tLINMsg *  - received lin (ascii) message
 * \param  [out] pMessage : tMessage * - decoded message
 *
 * \return bool - TRUE if frame is valid
 * \pre    none
 *
 * Uses    : none \n
 * Changes : none
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static bool lin_decode(tLINMsg * pLINMsg, tMessage * pMessage)
{
    U8 index;
    tLINMsg InputMsg;   // temporary buffer
    bool Result = TRUE;

    // convert LIN message from ASCII to hex digit
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

    // check length of the input message
    Result = Result && (0 == (InputMsg.Len % 2));

    // convert hex digits to bytes
    if (Result)
    {
        pMessage->Command = InputMsg.Data[0];
        pMessage->Address = mConvHexDigitsToByte(InputMsg.Data[1],InputMsg.Data[2]);
        pMessage->Len     = InputMsg.Data[3];

        // check length of the input message
        Result = (pMessage->Len == ((InputMsg.Len - 5)/2));

        // check checksum
        {
            U8 ChkSum = checksum(&(pLINMsg->Data[0]), pLINMsg->Len-2);
            Result = Result && (InputMsg.Data[InputMsg.Len-2] == mHiTetrade(ChkSum));
            Result = Result && (InputMsg.Data[InputMsg.Len-1] == mLoTetrade(ChkSum));
        }

        // pack hex digits
        if (Result)
        {
            for (index = 0; index < pMessage->Len; index++)
            {
                pMessage->Data[index] = mConvHexDigitsToByte(InputMsg.Data[4+(2*index)], InputMsg.Data[4+(2*index)+1]);
            }
        }
    }
    return Result;
}

/**
 * \fn static void lin_code( tMessage * pMessage, tLINMsg * pLINMsg)
 *
 * convert message from binary to ASCII format
 *
 * \param  [in]  pMessage : tMessage* - input message
 * \param  [out] pLINMsg  : tLINMsg*  - output message
 * \return none
 * \pre    none
 *
 * Uses    : none \n
 * Changes : none
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void lin_code( tMessage * pMessage, tLINMsg * pLINMsg)
{
    static U8 H2A[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    U8 index;
    U8 part;

    index = 0;
    pLINMsg->Data[index] = H2A[mLoTetrade(pMessage->Command)];
    index ++;
    pLINMsg->Data[index] = H2A[mHiTetrade(pMessage->Address)];
    index ++;
    pLINMsg->Data[index] = H2A[mLoTetrade(pMessage->Address)];
    index ++;
    pLINMsg->Data[index] = H2A[mLoTetrade(pMessage->Len)];
    index ++;
    for (part = 0; part < mLoTetrade(pMessage->Len); part++)
    {
        pLINMsg->Data[index] = H2A[mHiTetrade(pMessage->Data[part])];
        index++;
        pLINMsg->Data[index] = H2A[mLoTetrade(pMessage->Data[part])];
        index++;
    }

    // checksum
    {
        U8 ChkSum = checksum(&pLINMsg->Data[0],index);
        pLINMsg->Data[index] = H2A[mHiTetrade(ChkSum)];
        index ++;
        pLINMsg->Data[index] = H2A[mLoTetrade(ChkSum)];
        index ++;
    }
    pLINMsg->Len = index;
}

/**    @}
 * @}
 */

