/*
 * timer_funcs.c
 *
 *  Created on: 29/12/2023
 *      Author: carlo
 */

#include "timer_funcs.h"

// Queue handle
QueueHandle_t xTimerQueue;

// Counter variable
uint32_t counter = 0;

// Initialize Timer0A
void Timer0A_Init(void) {
    // Habilita o perif�rico do Timer 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    // Configura o Timer 0 para operar em modo peri�dico (up-count) e habilita o timer
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerEnable(TIMER0_BASE, TIMER_A);

    // Define o valor de recarga do Timer 0 para 10 segundo
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet());

    // Registra a fun��o de tratamento de interrup��o para o Timer 0
    TimerIntRegister(TIMER0_BASE, TIMER_A, timer_count_time);

    // Habilita a interrup��o de estouro do Timer 0
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Habilita as interrup��es globalmente
    IntMasterEnable();

}
//Fun��o que adiciona um segundo � contagem
void timer_count_time(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet());

    counter++;
    xQueueOverwriteFromISR(xTimerQueue, &counter, NULL);

    BaseType_t xHigherPriorityTaskWoken = pdTRUE;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
