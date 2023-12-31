/*
 * lcd_task.h
 *
 *  Created on: 21/12/2023
 *      Author: carlos almeida
 */

#ifndef LCD_TASK_H_
#define LCD_TASK_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "driverlib/gpio.h"
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "drivers/rgb.h"
#include "drivers/buttons.h"
#include "utils/uartstdio.h"
#include "keypad_task.h"
#include "i2c_temp.h"
#include "buzzer_pwm.h"
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
#include "uart_task.h"
#include "timers.h"
#include "timer_funcs.h"
#include <time.h>
#define LCDTASKSTACKSIZE        1536        // Stack size in words

#define RS GPIO_PIN_3 //Register Select (Character or Instruction)
#define EN GPIO_PIN_2 //LCD Clock Enable PIN, Falling Edge Triggered
// 4 bit operation
#define D4 GPIO_PIN_4 //Bit 4
#define D5 GPIO_PIN_5 //Bit 5
#define D6 GPIO_PIN_6 //Bit 6
#define D7 GPIO_PIN_7 //Bit 7


typedef struct Date
{
    int16_t month;
    int16_t day;
    int16_t year;
    int16_t hour;
    int16_t minute;
    int16_t second;
    int16_t starting_hour;
    int16_t starting_minute;
    int16_t starting_second;
} Date;

Date date;
int8_t initiated;

// function prototypes
void Lcd_Port(char a);
void Lcd_Cmd(char a);
void Lcd_Clear(void);
void Lcd_Set_Cursor(char a, char b);
void Lcd_Init(void);
void Lcd_Write_Char(char a);
void Lcd_Write_String(const char *a);
void Lcd_Shift_Right(void);
void Lcd_Shift_Left(void);
void Lcd_Write_Integer(int v);
void Lcd_Write_Float(float f);

extern uint32_t LCDTaskInit(void);
extern void LCDTask(void *pvParameters);

float packet_division(char *buffer, int number);
#endif /* LCD_TASK_H_ */
