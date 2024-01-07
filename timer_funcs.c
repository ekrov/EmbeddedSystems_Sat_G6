/*
 * timer_funcs.c
 *
 *  Created on: 29/12/2023
 *      Author: carlos
 */

#include "timer_funcs.h"

// Queue handle
QueueHandle_t xTimerQueue;

// Counter variable
uint32_t counter = 0;

// Initialize Timer0A
void Timer0A_Init(void) {
    // Enable of periferic of timer 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    // Configures timer 0 to operate in periodic more, and enables the timer itself
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerEnable(TIMER0_BASE, TIMER_A);

    // Defines the recharge of timer 0 to 10 seconds
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet());

    // Registers the interrupt handler of timer 0
    TimerIntRegister(TIMER0_BASE, TIMER_A, timer_count_time);

    // Enable of itnerrupt of timer 0
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Interrupt global enable
    IntMasterEnable();

}
//Function that adds a second to the counter and passes it on to a queue
void timer_count_time(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet());

    counter++;
    xQueueOverwriteFromISR(xTimerQueue, &counter, NULL);

    BaseType_t xHigherPriorityTaskWoken = pdTRUE;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
