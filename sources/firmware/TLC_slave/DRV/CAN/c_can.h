#ifndef C_CAN_H
#define C_CAN_H

#include "lpc_types.h"

// control bits for CAN_MSG_OBJ.mode_id
#define CAN_MSGOBJ_STD 0x00000000UL // CAN 2.0a 11-bit ID
#define CAN_MSGOBJ_EXT 0x20000000UL // CAN 2.0b 29-bit ID
#define CAN_MSGOBJ_DAT 0x00000000UL // data frame
#define CAN_MSGOBJ_RTR 0x40000000UL // rtr frame

typedef struct _CAN_MSG_OBJ
{
    uint32_t mode_id;
    uint32_t mask;
    uint8_t data[8];
    uint8_t dlc;
    uint8_t msgobj;
} CAN_MSG_OBJ;

typedef struct _CAN_ODCONSTENTRY
{
    uint16_t index;
    uint8_t subindex;
    uint8_t len;
    uint32_t val;
} CAN_ODCONSTENTRY;

// upper-nibble values for CAN_ODENTRY.entrytype_len
#define OD_NONE 0x00 // Object Dictionary entry doesn't exist
#define OD_EXP_RO 0x10 // Object Dictionary entry expedited, read-only
#define OD_EXP_WO 0x20 // Object Dictionary entry expedited, write-only
#define OD_EXP_RW 0x30 // Object Dictionary entry expedited, read-write
#define OD_SEG_RO 0x40 // Object Dictionary entry segmented, read-only
#define OD_SEG_WO 0x50 // Object Dictionary entry segmented, write-only
#define OD_SEG_RW 0x60 // Object Dictionary entry segmented, read-write

typedef struct _CAN_ODENTRY
{
    uint16_t index;
    uint8_t subindex;
    uint8_t entrytype_len;
    uint8_t isr_handled;
    uint8_t *val;
} CAN_ODENTRY;

typedef struct _CAN_CANOPENCFG
{
    uint8_t node_id;
    uint8_t msgobj_rx;
    uint8_t msgobj_tx;
    uint32_t od_const_num;
    CAN_ODCONSTENTRY *od_const_table;
    uint32_t od_num;
    CAN_ODENTRY *od_table;
} CAN_CANOPENCFG;

typedef struct _CAN_CALLBACKS
{
    void (*CAN_rx)(uint8_t msg_obj);
    void (*CAN_tx)(uint8_t msg_obj);
    void (*CAN_error)(uint32_t error_info);
    uint32_t (*CANOPEN_sdo_read)(uint16_t index, uint8_t subindex);
    uint32_t (*CANOPEN_sdo_write)(
    uint16_t index, uint8_t subindex, uint8_t *dat_ptr);
    uint32_t (*CANOPEN_sdo_seg_read)(
    uint16_t index, uint8_t subindex, uint8_t openclose,
    uint8_t *length, uint8_t *data, uint8_t *last);
    uint32_t (*CANOPEN_sdo_seg_write)(
    uint16_t index, uint8_t subindex, uint8_t openclose,
    uint8_t length, uint8_t *data, uint8_t *fast_resp);
    uint8_t (*CANOPEN_sdo_req)(
    uint8_t length_req, uint8_t *req_ptr, uint8_t *length_resp,
    uint8_t *resp_ptr);
} CAN_CALLBACKS;

// error status bits
#define CAN_ERROR_NONE 0x00000000UL
#define CAN_ERROR_PASS 0x00000001UL
#define CAN_ERROR_WARN 0x00000002UL
#define CAN_ERROR_BOFF 0x00000004UL
#define CAN_ERROR_STUF 0x00000008UL
#define CAN_ERROR_FORM 0x00000010UL
#define CAN_ERROR_ACK  0x00000020UL
#define CAN_ERROR_BIT1 0x00000040UL
#define CAN_ERROR_BIT0 0x00000080UL
#define CAN_ERROR_CRC  0x00000100UL

typedef struct _CAND
{
    void (*init_can) (uint32_t * can_cfg, uint8_t isr_ena);
    void (*isr) (void);
    void (*config_rxmsgobj) (CAN_MSG_OBJ * msg_obj);
    uint8_t (*can_receive) (CAN_MSG_OBJ * msg_obj);
    void (*can_transmit) (CAN_MSG_OBJ * msg_obj);
    void (*config_canopen) (CAN_CANOPENCFG * canopen_cfg);
    void (*canopen_handler) (void);
    void (*config_calb) (CAN_CALLBACKS * callback_cfg);
} CAND;

typedef struct _ROM
{
    const unsigned p_usbd;
    const unsigned p_clib;
    const CAND    * pCAND;
    const unsigned p_pwrd;
    const unsigned p_dev1;
    const unsigned p_dev2;
    const unsigned p_dev3;
    const unsigned p_dev4;
} ROM;

ROM **rom = (ROM **)0x1fff1ff8;

#define C_CAN_PTR   (*rom)->pCAND

#endif
