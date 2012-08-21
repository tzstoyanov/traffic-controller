#include "lpc11xx_gpio.h"
#include "lpc11xx_tmr.h"


#include "types.h"
#include "system.h"
#include "signals.h"
#include "os.h"

#include "nvram.h"
#include "lin.h"

extern void appl_init(void);

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
    appl_init();

    OSSetPeriodicAlarm(cOS_TaskId_TimerTask, 2);

    /* start OS tick timer */
    SysTick_Config(cOSTickInTicks);
}

void IdleTask(void)
{
}

