/*
 * keypad_task.h
 *
 *  Created on: 21/12/2023
 *      Author: carlo
 */

#ifndef KEYPAD_TASK_H_
#define KEYPAD_TASK_H_



#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "drivers/rgb.h"
#include "drivers/buttons.h"
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


//--------------Keypad defining--------------//
#define num_Rows    4
#define num_Cols    4
#define Row_0       GPIO_PIN_4
#define Row_1       GPIO_PIN_3
#define Row_2       GPIO_PIN_2
#define Row_3       GPIO_PIN_0
#define Col_0       GPIO_PIN_2
#define Col_1       GPIO_PIN_3
#define Col_2       GPIO_PIN_4
#define Col_3       GPIO_PIN_5
#define Rows        Row_0|Row_1|Row_2|Row_3
#define Cols        Col_0|Col_1|Col_2|Col_3
#define Rows_port   GPIO_PORTE_BASE
#define Cols_port   GPIO_PORTA_BASE



#define KEYPADSTACKSIZE        256         // Stack size in words
#define KEYPAD_ITEM_SIZE           sizeof(uint8_t)
#define KEYPAD_QUEUE_SIZE          5



//---------------Function declarations------------//
uint8_t Check_KPad();       //Check if any sw are pressed
uint8_t Get_Key();          //Return the key that is pressed
void UART0_Init();          //UART 0 initialization communicate with PC
void GPIO_Keypad_Init();    //GPIO initialization for keypad

extern uint32_t KeypadTaskInit(void);
extern void KeypadTask(void *pvParameters);

#endif /* KEYPAD_TASK_H_ */
