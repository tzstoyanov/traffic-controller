/**
 * \file
 * \project TLC
 * \brief   simple CAN driver (code)
 *
 * \details
 * The file implements a basic CAN RX and TX queue
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
 * 20.08.2012 taa: UART_TRACE_CAN moved to system.h
 *
 *//* ************************************************************************ */

/*
 * include files
 */

#include "LPC11xx.h"    // register access
#include "types.h"      // user types

#include "c_can.h"
#include "can.h"
#include "can_cfg.h"

#include "tst.h"        // for trace
#include "system.h"     // system definition
#include "os.h"

/*
 * local defines
 */


#define cCanMaxErrorCounterValue    0xFFFF


/*
 * local types
 */

/** CAN Queue type */
typedef struct
{
    tCanMsg Message[cCANQueueSize];   /**<  message buffer */
    U8 IndexIn;                         /**< input index */
    U8 IndexOut;                        /**< output index */
    bool IsFull;                        /**< buffer is full flag */
} tCANQueue;

/** CAN driver parameters */
typedef struct
{
    U16  ErrTxTimeoutCnt;
    U16  ErrTxOverflowCnt;
    U16  ErrRxOverflowCnt;
    U16  ErrBusOffCnt;

    U16  TxTimeoutCnt;      /** TX timeout counter */
    bool BusOff;            /**< bus-off signal flag */
} tCANData;

/*
 * local variables
 */

static tCANQueue CANTxQueue;    /**< TX queue */
static tCANQueue CANRxQueue;    /**< RX queue */
static tCANData  CANData;                        /**< component data */

/** callbacks structure used in can initialization */
static void can_rx_callback(U8 MsgNo);
    /**< called after successful can reception */
static void can_tx_callback(U8 MsgNo);
    /**< called after successful can transmission */
static void can_err_callback(U32 Error);
    /**< called in case of can error */
const CAN_CALLBACKS CANCallbacks =
{
    can_rx_callback,
    can_tx_callback,
    can_err_callback,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

const U32 CanApiClkInitTable[2] = cCanInitTableInitializer;
    /**< CAN cell settings */

static const tpCANRxCallback pCANRxCallback = cCANRxCallback;
    /**< reception callout function */

/*
 * local function prototypes
 */

static void can_cell_init(void);
    /**< CAN cell initialization */
static can_internal_send (tCanMsg * pCanMsg);
    /**< internal send routine */

/*
 * exported functions
 */

/** @fn void can_init(void)
 * initialization routine
 *
 * @param  none
 * @return none
 * @pre    component is initialized
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void can_init(void)
{
    // internal variables initialization
    CANTxQueue.IndexIn  = 0;
    CANTxQueue.IndexOut = 0;
    CANTxQueue.IsFull   = 0;

    CANRxQueue.IndexIn  = 0;
    CANRxQueue.IndexOut = 0;
    CANRxQueue.IsFull   = 0;

    CANData.BusOff = FALSE;

    CANData.ErrTxTimeoutCnt  = 0;
    CANData.ErrTxOverflowCnt = 0;
    CANData.ErrRxOverflowCnt = 0;
    CANData.ErrBusOffCnt     = 0;

    // can cell initialization
    can_cell_init();

    // start of the periodic task
    OSSetPeriodicAlarm(cOS_TaskId_CanTask, 2);

#ifdef UART_TRACE_CAN
    TSTOutStr("\n\rCAN init");
#endif
}

/** @fn void CAN_IRQHandler(void)
 * can RX ISR
 *
 * @param  none
 * @return none
 * @pre    component is initialized
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CAN_IRQHandler(void)
{
    C_CAN_PTR->isr();
}

/** @fn void can_task(void)
 * CAN periodic task
 *
 * @param  none
 * @return none
 * @pre    component is initialized
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void can_task(void)
{
    if (CANData.BusOff)
    { // try to reinitialize the CAN cell
        can_cell_init();
    }

    if (CANData.TxTimeoutCnt >0)
    { // TX is pending
        CANData.TxTimeoutCnt--;
        if (0 == CANData.TxTimeoutCnt)
        { // cancel transmission and go to the next message
            // TODO: cancell current transmission ?!?
            if (CANData.ErrTxTimeoutCnt < cCanMaxErrorCounterValue)
            {
                CANData.ErrTxTimeoutCnt++;
            }
            OSActivateTask(cOS_TaskId_CanTxTask);
        }
    }
}

/** @fn void can_tx_task(void)
 * CAN TX task
 *
 * @param  none
 * @return none
 * @pre    component is initialized
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void can_tx_task(void)
{
    if ((CANTxQueue.IndexIn != CANTxQueue.IndexOut) || (CANTxQueue.IsFull))
    {
        can_internal_send(&CANTxQueue.Message[CANTxQueue.IndexOut]);
        CANTxQueue.IndexOut++;
        if (CANTxQueue.IndexOut >= cCANQueueSize)
        {
            CANTxQueue.IndexOut = 0;
        }
        CANTxQueue.IsFull = FALSE;
    }
}

/** @fn void can_rx_task(void)
 * CAN reception task
 *
 * @param  none
 * @return none
 * @pre    component is initialized
 *
 * Uses: \n
 * Changes:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void can_rx_task(void)
{
    if ((CANRxQueue.IndexIn != CANRxQueue.IndexOut) || (CANRxQueue.IsFull))
    {
        if (pCANRxCallback != NULL)
        {
            pCANRxCallback(&CANRxQueue.Message[CANRxQueue.IndexOut]);
            CANRxQueue.IndexOut++;
            if (CANRxQueue.IndexOut >= cCANQueueSize)
            {
                CANRxQueue.IndexOut = 0;
            }
            CANRxQueue.IsFull = FALSE;
        }
    }

    if ((CANRxQueue.IndexIn != CANRxQueue.IndexOut) || (CANRxQueue.IsFull))
    { // more data are available
        OSActivateTask(cOS_TaskId_CanRxTask);
    }
}

/**
 * \fn void can_send(tCanMsg * pCanMsg)
 *
 * call to send CAN message
 *
 * \param  [in] pCanMsg : tCanMsg * - message to be send
 * \return TRUE if message is put into the queue
 * \pre    component is initialized
 *
 * Uses    : CANTxQueue \n
 * Changes : CANTxQueue
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
bool can_send(tCanMsg * pCanMsg)
{
    bool Result = FALSE;

    if (!CANTxQueue.IsFull)
    { // queue is not full
        CANTxQueue.Message[CANTxQueue.IndexIn] = *pCanMsg;
        CANTxQueue.IndexIn++;
        if (CANTxQueue.IndexIn >= cCANQueueSize)
        {
            CANTxQueue.IndexIn = 0;
        }
        if (CANTxQueue.IndexIn == CANTxQueue.IndexOut)
        {
            CANTxQueue.IsFull = TRUE;
        }
        OSActivateTask(cOS_TaskId_CanTxTask);
        Result = TRUE;
    }
    else
    { // queue is full
        if (CANData.ErrTxOverflowCnt < cCanMaxErrorCounterValue)
        {
            CANData.ErrTxOverflowCnt++;
        }
    }

    return Result;
}

static void can_rx_callback(U8 MsgNo)
{
    CAN_MSG_OBJ msg_obj;
    U8 str[128];
    U8 index;

    msg_obj.msgobj = MsgNo;
    C_CAN_PTR->can_receive(&msg_obj);

    if (!CANRxQueue.IsFull)
    { // put into the queue
        CANRxQueue.Message[CANRxQueue.IndexIn].ID     = msg_obj.mode_id;
        CANRxQueue.Message[CANRxQueue.IndexIn].Length = msg_obj.dlc;
        for (index=0; index < msg_obj.dlc; index++)
        {
            CANRxQueue.Message[CANRxQueue.IndexIn].Data[index] = msg_obj.data[index];
        }
        CANRxQueue.IndexIn++;
        if (CANRxQueue.IndexIn >= cCANQueueSize)
        {
            CANRxQueue.IndexIn = 0;
        }
        if (CANRxQueue.IndexIn == CANRxQueue.IndexOut)
        {
            CANRxQueue.IsFull = TRUE;
        }
        OSActivateTask(cOS_TaskId_CanRxTask);
    }
    else
    { // queue is full
        if (CANData.ErrRxOverflowCnt < cCanMaxErrorCounterValue)
        {
            CANData.ErrRxOverflowCnt++;
        }
    }

#ifdef UART_TRACE_CAN
    TSTOutStr("\r\nRX:");
    TSTOut16H(msg_obj.mode_id);

    for (index=0; index < msg_obj.dlc; index++)
    {
        TSTOutStr(":");
        TSTOut8H(msg_obj.data[index]);
    }
#endif
}

/**
 * \fn static void can_tx_callback(U8 MsgNo)
 *
 * end of transmission callback
 *
 * \param  [in] MsgNo : U8 - message box number
 * \return none
 * \pre    none
 *
 * Uses    : CANData.TxTimeoutCnt \n
 * Changes : none
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static void can_tx_callback(U8 MsgNo)
{
    if (CANData.TxTimeoutCnt > 0)
    {
        OSActivateTask(cOS_TaskId_CanTxTask);
    }
}

/**
 * \fn static void can_err_callback(U32 Error)
 *
 * error callback
 *
 * signal the buss-off state, if detected
 *
 * \param  [in] Error : U32 - error type, see c_can.h
 * \return none
 * \pre    none
 *
 * Uses    :none \n
 * Changes : CANData.BusOff
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void can_err_callback(U32 Error)
{
#ifdef UART_TRACE_CAN
    TSTOutStr("\n\rERR:");
    TSTOut32H(Error);
#endif
    if (0 != (Error & CAN_ERROR_BOFF))
    {
        CANData.BusOff = TRUE;
        if (CANData.ErrBusOffCnt < cCanMaxErrorCounterValue)
        {
            CANData.ErrBusOffCnt++;
        }
    }
}

/**
 * \fn static can_internal_send (tCanMsg * pCanMsg)
 *
 * internal CAN send routine
 *
 * \param  [in] pCanMsg: tCanMsg - message to be send
 * \return none
 * \pre    none
 *
 * Uses    : none \n
 * Changes : none
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static can_internal_send (tCanMsg * pCanMsg)
{
    CAN_MSG_OBJ msg_obj;
    U8 index;

    msg_obj.msgobj  = cCANTxMsgBox;
    msg_obj.mode_id = pCanMsg->ID;
    msg_obj.mask    = 0x07FFUL;
    msg_obj.dlc     = pCanMsg->Length;
    msg_obj.data[0] = pCanMsg->Data[0];
    msg_obj.data[1] = pCanMsg->Data[1];
    msg_obj.data[2] = pCanMsg->Data[2];
    msg_obj.data[3] = pCanMsg->Data[3];
    msg_obj.data[4] = pCanMsg->Data[4];
    msg_obj.data[5] = pCanMsg->Data[5];
    msg_obj.data[6] = pCanMsg->Data[6];
    msg_obj.data[7] = pCanMsg->Data[7];

    C_CAN_PTR->can_transmit(&msg_obj);

    // reload timeout
    CANData.TxTimeoutCnt = cCANTxTimeoutInMs/cCANTickinMs;

#ifdef UART_TRACE_CAN
    TSTOutStr("\n\rTX:");
    TSTOut16H(msg_obj.mode_id);

    for (index=0; index < msg_obj.dlc; index++)
    {
        TSTOutStr(":");
        TSTOut8H(msg_obj.data[index]);
    }
#endif
}


/**
 * \fn static void can_cell_init(void)
 *
 * can cell initialization
 *
 * \param  none
 * \return none
 * \pre    none
 *
 * Uses    : <USES> \n
 * Changes : can registers
 *
 *//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static void can_cell_init(void)
{
    U8 index;
    CAN_MSG_OBJ msg_obj;

    LPC_CAN->CNTL = 0x01;

    C_CAN_PTR->init_can(&CanApiClkInitTable[0], TRUE);

    // Configure message objects
    msg_obj.msgobj   = 1;
    msg_obj.mode_id = 0x000;
    msg_obj.mask    = 0x000;
    for (index=0; index<30; index++ )
    {
        C_CAN_PTR->config_rxmsgobj(&msg_obj);
    }

    // set call-backs
    C_CAN_PTR->config_calb(&CANCallbacks);

#ifdef CAN_TEST
    LPC_CAN->CNTL = LPC_CAN->CNTL | 0x80;
    LPC_CAN->TEST = 0x18; // LB mode
#endif

    NVIC_EnableIRQ(CAN_IRQn);

    CANData.BusOff = 0;
}

/** \}\} */
/*** End of file **************************************************************/
