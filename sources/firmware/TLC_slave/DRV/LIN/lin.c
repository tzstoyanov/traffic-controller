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
    //UART_Send(LPC_UART, LINRxMsg.Data, LINRxMsg.Len, BLOCKING);
    if (NULL != LINData.pRxCallback)
    {
        LINData.pRxCallback(&LINRxMsg);
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

/**    @}
 * @}
 */

