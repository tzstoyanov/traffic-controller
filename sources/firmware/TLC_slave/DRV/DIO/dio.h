#ifndef DIO_H
#define DIO_H

#include "lpc11xx_gpio.h"

#define DIO_POLARITY_POSITIVE   0
#define DIO_POLARITY_NEGATIVE   1

#define mDio_SetPin(OUT, STATE)     mDio_SetPinx(OUT, STATE)
#define mDio_SetPinx(OUT, STATE)    mDio_SetPin_int(PORT_##OUT, PIN_##OUT, STATE, POL_##OUT)

#define mDio_GetPin(IN, STATE)      mDio_GetPinx(IN, STATE)
#define mDio_GetPinx(IN, STATE)     mDio_GetPin_int(PORT_##IN, PIN_##IN, STATE, POL_##IN)

    #define PORT_LedTx  1
    #define PIN_LedTx   GPIO_Pin_10
    #define POL_LedTx   DIO_POLARITY_NEGATIVE

    #define PORT_LedRx  1
    #define PIN_LedRx   GPIO_Pin_11
    #define POL_LedRx   DIO_POLARITY_NEGATIVE

#if DEVICE_TYPE == DEVICE_TYPE_LAMP
    #define PORT_GREEN  1
    #define PIN_GREEN   GPIO_Pin_9
    #define POL_GREEN   DIO_POLARITY_NEGATIVE
#endif


#define mDio_SetPin_int(PORT, PIN, STATE, POL)   \
{ if ((POL) != (STATE))                     \
  { mGPIO_SetBits(PORT, PIN); }             \
  else                                      \
  { mGPIO_ResetBits(PORT, PIN); }           \
}

#define mDio_GetPin_int(PORT, PIN, POL)  (POL) != (GPIO_ReadInputPin(PORT, PIN))

#define mGPIO_SetBits(PORT, PIN)                \
 ((uint32_t *) mGPIO_GetPointer(PORT))[PIN] = PIN;

#define mGPIO_ResetBits(PORT, PIN)              \
((uint32_t *) mGPIO_GetPointer(PORT))[PIN] = 0;

// PORT definitions
//////////////////////////////////////////////////////////

#define mGPIO_GetPointer(PORT)   mGPIO_GetPointerx(PORT)
#define mGPIO_GetPointerx(PORT)  mGPIO_GetPointer_##PORT

#define mGPIO_GetPointer_0  LPC_GPIO0
#define mGPIO_GetPointer_1  LPC_GPIO1
#define mGPIO_GetPointer_2  LPC_GPIO2
#define mGPIO_GetPointer_3  LPC_GPIO3

#endif
