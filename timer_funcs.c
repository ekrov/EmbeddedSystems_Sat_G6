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
static uint32_t counter = 0;

// Initialize Timer0A
void Timer0A_Init(void) {
    // Enable the Timer0 peripheral
    SYSCTL_RCGCTIMER_R |= 0x01;

    // Disable Timer0A before configuration
    TIMER0_CTL_R = 0x00000000;

    // Configure Timer0A in periodic mode
    TIMER0_CFG_R = 0x00;  // 32-bit mode
    TIMER0_TAMR_R = 0x02;  // Periodic mode
    TIMER0_TAILR_R = 16000000 - 1;  // Assuming a 16MHz clock, generates 1Hz interrupts

    // Enable Timer0A interrupt
    TIMER0_IMR_R |= 0x01;
    NVIC_EN0_R |= 1 << 19;  // Enable Timer0A interrupt in NVIC

    // Set the priority of the Timer0A interrupt
    NVIC_PRI4_R = (NVIC_PRI4_R & 0xFFFFFF00) | 0x20;  // Adjust priority as needed

    // Enable Timer0A
    TIMER0_CTL_R |= 0x01;
}

// Timer callback function
void vTimerCallback(TimerHandle_t xTimer) {
    // Increment a value and send it to the queue
    counter++;
    xQueueSendToBackFromISR(xTimerQueue, &counter, NULL);
}
