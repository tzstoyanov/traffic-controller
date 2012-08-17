#include "lpc11xx_gpio.h"
#include "lpc11xx_tmr.h"


#include "types.h"
#include "system.h"
#include "signals.h"
#include "os.h"

#include "nvram.h"
#include "gateway.h"
#include "lamp.h"
#include "lin.h"


/*
 * constants
 */

#define LED_SET(LED, STATE)     LED_SETx(LED, STATE)
#define LED_SETx(LED,STATE)     LED_##LED##_##STATE

#define LED_0_0     GPIO_SetBits(PORT1, GPIO_Pin_10)
#define LED_0_1     GPIO_ResetBits(PORT1, GPIO_Pin_10)
#define LED_1_0     GPIO_SetBits(PORT1, GPIO_Pin_11)
#define LED_1_1     GPIO_ResetBits(PORT1, GPIO_Pin_11)
#define LED_2_0     GPIO_SetBits(PORT1, GPIO_Pin_9)
#define LED_2_1     GPIO_ResetBits(PORT1, GPIO_Pin_9)

#define LED1    0
#define LED2    1
#define LEDG    2

#define LED_GET(LED)     LED_GETx(LED)
#define LED_GETx(LED)    LED_GET_##LED

#define LED_GET_0    !GPIO_ReadInputPin(PORT1, GPIO_Pin_10)
#define LED_GET_1    !GPIO_ReadInputPin(PORT1, GPIO_Pin_11)
#define LED_GET_2    !GPIO_ReadInputPin(PORT1, GPIO_Pin_9)

#define TOGGLE_LED(LED) if (LED_GET(LED)) { LED_SET(LED,0); } else {LED_SET(LED,1);}

// PORT definitions
//////////////////////////////////////////////////////////

#define mGPIO_GetPointer(PORT)	 mGPIO_GetPointerx(PORT)
#define mGPIO_GetPointerx(PORT)  mGPIO_GetPointer_##PORT

#define mGPIO_GetPointer_0	LPC_GPIO0
#define mGPIO_GetPointer_1	LPC_GPIO1
#define mGPIO_GetPointer_2	LPC_GPIO2
#define mGPIO_GetPointer_3	LPC_GPIO3

// port set definitions

//#define mGPIO_SetBits(PORT, PIN)   				\
//{ GPIO_TypeDef *pGPIO = mGPIO_GetPointer(PORT);	\
//  pGPIO->MASKED_ACCESS[PIN & GPIO_PINS_MASK] = PIN & GPIO_PINS_MASK;\
//}

//#define mGPIO_SetBits(PORT, PIN)   				\
// ((GPIO_TypeDef *) mGPIO_GetPointer(PORT))->MASKED_ACCESS[PIN & GPIO_PINS_MASK] = PIN & GPIO_PINS_MASK;
//
//
//#define mGPIO_ResetBits(PORT, PIN)   				\
//((GPIO_TypeDef *) mGPIO_GetPointer(PORT))->MASKED_ACCESS[PIN & GPIO_PINS_MASK] = 0;


#define mGPIO_SetBits(PORT, PIN)   				\
 ((uint32_t *) mGPIO_GetPointer(PORT))[PIN] = PIN;


#define mGPIO_ResetBits(PORT, PIN)   				\
((uint32_t *) mGPIO_GetPointer(PORT))[PIN] = 0;

///////////////////////////////////////////////////////////////

#define SYST_CSR        0xE000E010		// System Timer Control and status register
#define SYST_RVR        0xE000E014      // System Timer Reload value register
#define SYST_CVR        0xE000E018      // System Timer Current value register
#define SYST_CALIB      0xE000E01C      // System Timer Calibration value register

int main(void)
{
    /* start OS */
    OSStart();

    // TODO: assert
    return -1;
}

void SysTick_Handler(void)
{
    /* call OS tick */
    OSTick();
    lin_task();
}

void OSStarupHook(void)
{
    GPIO_SetDir(PORT1, GPIO_Pin_9 , 1);
    GPIO_SetDir(PORT1, GPIO_Pin_10, 1);
    GPIO_SetDir(PORT1, GPIO_Pin_11, 1);

    nvram_init();

    // TODO: move to applstate manager
#if DEVICE_TYPE == DEVICE_TYPE_GATEWAY
    gw_init();
#elif DEVICE_TYPE == DEVICE_TYPE_LAMP
    lamp_init();
#endif

    OSSetPeriodicAlarm(cOS_TaskId_TimerTask, 2);

    /* start OS tick timer */
    SysTick_Config(cOSTickInTicks);
}

void IdleTask(void)
{
}

