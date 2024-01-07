/*
 * i2c_temp.h
 *
 *  Created on: 22/12/2023
 *      Author: carlo
 */

#ifndef I2C_TEMP_H_
#define I2C_TEMP_H_

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

#include "inc/hw_i2c.h"
#include "inc/hw_gpio.h"
#include "driverlib/i2c.h"


#define TMP101_ADDRESS 0x49
#define TMP101_config_REG 0b00000001
#define TMP101_TEMP_REG 0b00000000

#define TEMPSTACKSIZE        100         // Stack size in words

#define I2CTEMP_ITEM_SIZE           sizeof(float)
#define I2CTEMP_QUEUE_SIZE          1


void InitI2C0();
void I2CSend(uint8_t slave_addr, uint8_t num_of_args, ...);
void I2CSendString(uint32_t slave_addr, char array[]);
uint32_t I2CReceive(uint32_t slave_addr, uint8_t reg);
void I2CSendByte(uint8_t slave_addr, uint32_t byte);
uint32_t I2C_write8BitRegister(uint8_t slaveAddress, uint32_t addressPointer, uint8_t firstByte);
uint32_t I2C_read16BitRegister(uint8_t slaveAddress, uint8_t registerAddress);

extern uint32_t I2cTempTaskInit(void);
extern void I2cTempTask(void *pvParameters);

#endif /* I2C_TEMP_H_ */
