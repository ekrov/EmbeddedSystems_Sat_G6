/*
 * timer_funcs.h
 *
 *  Created on: 29/12/2023
 *      Author: carlo
 */

#ifndef TIMER_FUNCS_H_
#define TIMER_FUNCS_H_



#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "utils/uartstdio.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdarg.h>
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "inc/hw_i2c.h"
#include "inc/hw_gpio.h"
#include "driverlib/i2c.h"
#include "inc/hw_ints.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "timers.h"
#include "inc/tm4c123gh6pm.h"  // TivaWare header for TM4C123GXL


// Timer callback
void vTimerCallback(TimerHandle_t xTimer);
void Timer0A_Init(void);



#endif /* TIMER_FUNCS_H_ */
