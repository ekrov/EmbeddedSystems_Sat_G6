/*
 * buzzer_pwm.h
 *
 *  Created on: 22/12/2023
 *      Author: carlo
 */

#ifndef BUZZER_PWM_H_
#define BUZZER_PWM_H_


#define BUZZERSTACKSIZE        100         // Stack size in words


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

uint32_t BuzzerTaskInit(void);
void configurePWM(void);
void playSong(void);
static void BuzzerTask(void *pvParameters);

#endif /* BUZZER_PWM_H_ */
