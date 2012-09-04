#ifndef LIN_H
#define LIN_H

#define cLINCmdMaxLength    24


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



typedef void ( * tpLINRxCallBack) (tMessage * pMessage);

extern void lin_init(void);
extern void lin_task(void);
extern void lin_rx_task(void);
//extern void lin_send(tLINMsg * pLINMsg);
extern void lin_send_message(tMessage * pMessage);

#endif
