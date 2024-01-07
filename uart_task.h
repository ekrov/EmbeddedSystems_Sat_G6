/*
 * uart_task.h
 *
 *  Created on: 22/12/2023
 *      Author: carlo
 */

#ifndef UART_TASK_H_
#define UART_TASK_H_


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

// Define o tamanho da queue uart
#define UART_QUEUE_LENGTH 20
#define UART_QUEUE_counter_LENGTH 1

#define UARTSTACKSIZE 200

#define buffer_size 70
// Define o tamanho dos itens na queue uart
#define UART_SIZE buffer_size*sizeof(char)
#define UART_counter_SIZE 1*sizeof(int16_t)



//int32_t UART_receive(void);
void UART_Init(void);
uint32_t UartTaskReceiveInit(void);
static void UartTaskReceive(void *pvParameters);
void UARTIntHandler(void);

#endif /* UART_TASK_H_ */
