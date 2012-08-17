#ifndef LIN_H
#define LIN_H

#define cLINCmdMaxLength    24

typedef struct
{
    U8 Data[cLINCmdMaxLength];
    U8 Len;
} tLINMsg;

typedef void ( * tpLINRxCallBack) (tLINMsg * pLINMsg);

extern void lin_init(void);
extern void lin_task(void);
extern void lin_rx_task(void);
extern void lin_send(tLINMsg * pLINMsg);

#endif
