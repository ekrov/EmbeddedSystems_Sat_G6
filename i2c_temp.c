/*
 * i2c_tmp.c
 *
 *  Created on: 14/12/2023
 *      Author: carlos Almeida
 */


#include "i2c_temp.h"

xQueueHandle g_pI2cTempQueue;
float temperature=0;
float temperature_conv=0;
uint32_t command_tmp101=0b01100000;

uint8_t i;
//initialize I2C module 0
//Slightly modified version of TI's example code
void InitI2C0(void)
{
    //enable I2C module 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

    //reset module
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);

    //enable GPIO peripheral that contains I2C 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // Configure the pin muxing for I2C0 functions on port B2 and B3.
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    // Select the I2C function for these pins.
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    // Enable and initialize the I2C0 master module.  Use the system clock for
    // the I2C0 module.  The last parameter sets the I2C data transfer rate.
    // If false the data rate is set to 100kbps and if true the data rate will
    // be set to 400kbps.
    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), true);

    //clear I2C FIFOs
    //HWREG(I2C0_BASE + I2C_O_FIFOCTL) = 80008000;
}

//sends an I2C command to the specified slave
void I2CSend(uint8_t slave_addr, uint8_t num_of_args, ...)
{
    // Tell the master module what address it will place on the bus when
    // communicating with the slave.
    I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, false);

    //stores list of variable number of arguments
    va_list vargs;

    //specifies the va_list to "open" and the last fixed argument
    //so vargs knows where to start looking
    va_start(vargs, num_of_args);

    //put data to be sent into FIFO
    I2CMasterDataPut(I2C0_BASE, va_arg(vargs, uint32_t));

    //if there is only one argument, we only need to use the
    //single send I2C function
    if(num_of_args == 1)
    {
        //Initiate send of data from the MCU
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);

        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));

        //"close" variable argument list
        va_end(vargs);
    }

    //otherwise, we start transmission of multiple bytes on the
    //I2C bus
    else
    {
        //Initiate send of data from the MCU
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);

        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));

        //send num_of_args-2 pieces of data, using the
        //BURST_SEND_CONT command of the I2C module
        for(i = 1; i < (num_of_args - 1); i++)
        {
            //put next piece of data into I2C FIFO
            I2CMasterDataPut(I2C0_BASE, va_arg(vargs, uint32_t));
            //send next data that was just placed into FIFO
            I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);

            // Wait until MCU is done transferring.
            while(I2CMasterBusy(I2C0_BASE));
        }

        //put last piece of data into I2C FIFO
        I2CMasterDataPut(I2C0_BASE, va_arg(vargs, uint32_t));
        //send next data that was just placed into FIFO
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));

        //"close" variable args list
        va_end(vargs);
    }
}



//sends an array of data via I2C to the specified slave
void I2CSendString(uint32_t slave_addr, char array[])
{
    // Tell the master module what address it will place on the bus when
    // communicating with the slave.
    I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, false);

    //put data to be sent into FIFO
    I2CMasterDataPut(I2C0_BASE, array[0]);

    //if there is only one argument, we only need to use the
    //single send I2C function
    if(array[1] == '\0')
    {
        //Initiate send of data from the MCU
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);

        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));
    }

    //otherwise, we start transmission of multiple bytes on the
    //I2C bus
    else
    {
        //Initiate send of data from the MCU
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);

        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));

        //initialize index into array
        uint8_t i = 1;

        //send num_of_args-2 pieces of data, using the
        //BURST_SEND_CONT command of the I2C module
        while(array[i + 1] != '\0')
        {
            //put next piece of data into I2C FIFO
            I2CMasterDataPut(I2C0_BASE, array[i++]);

            //send next data that was just placed into FIFO
            I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);

            // Wait until MCU is done transferring.
            while(I2CMasterBusy(I2C0_BASE));
        }

        //put last piece of data into I2C FIFO
        I2CMasterDataPut(I2C0_BASE, array[i]);

        //send next data that was just placed into FIFO
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

        // Wait until MCU is done transferring.
        while(I2CMasterBusy(I2C0_BASE));
    }
}

//read specified register on slave device
uint32_t I2CReceive(uint32_t slave_addr, uint8_t reg)
{
    //specify that we are writing (a register address) to the
    //slave device
    I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, false);

    //specify register to be read
    I2CMasterDataPut(I2C0_BASE, reg);

    //send control byte and register address byte to slave device
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    //wait for MCU to finish transaction
    while(I2CMasterBusy(I2C0_BASE));

    //specify that we are going to read from slave device
    I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, true);

    //send control byte and read from the register we
    //specified
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);

    //wait for MCU to finish transaction
    while(I2CMasterBusy(I2C0_BASE));

    //return data pulled from the specified register
    return I2CMasterDataGet(I2C0_BASE);
}


//sends an I2C command to the specified slave
void I2CSendByte(uint8_t slave_addr, uint32_t byte)
{
    // Tell the master module what address it will place on the bus when
    // communicating with the slave.
    I2CMasterSlaveAddrSet(I2C0_BASE, slave_addr, false);


    //put data to be sent into FIFO
    I2CMasterDataPut(I2C0_BASE, byte);


    //Initiate send of data from the MCU
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    // Wait until MCU is done transferring.
    while(I2CMasterBusy(I2C0_BASE));

}

uint32_t I2C_write8BitRegister(uint8_t slaveAddress, uint32_t addressPointer, uint8_t firstByte)
{
    I2CMasterSlaveAddrSet(I2C0_BASE, slaveAddress, false);
    I2CMasterDataPut(I2C0_BASE, addressPointer);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    while(I2CMasterBusy(I2C0_BASE))
    {
    }
    I2CMasterDataPut(I2C0_BASE, firstByte);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
    while(I2CMasterBusy(I2C0_BASE))
    {
    }

    return 0;
}

uint32_t I2C_read16BitRegister(uint8_t slaveAddress, uint8_t registerAddress)
{

    I2CMasterSlaveAddrSet(I2C0_BASE, slaveAddress, false); // set slave address
    I2CMasterDataPut(I2C0_BASE, registerAddress); // set register to be read
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);
    while(I2CMasterBusy(I2C0_BASE))
    {
    }

    /*---------------------------------READ-Section----------------------------------------*/

    I2CMasterSlaveAddrSet(I2C0_BASE, slaveAddress, true); // set slave address

    //read first Byte
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
    while(I2CMasterBusy(I2C0_BASE))
    {
    }
    uint32_t msb=I2CMasterDataGet(I2C0_BASE);

    //read second Byte
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
    while(I2CMasterBusy(I2C0_BASE))
    {
    }
    uint32_t lsb=I2CMasterDataGet(I2C0_BASE);

    return ((msb << 8) | lsb);
}


static void
I2cTempTask(void *pvParameters)
{
    portTickType ui32WakeTime;
    uint32_t ui32LEDToggleDelay;
    uint8_t i8Message;

    //
    // Get the current tick count.
    //
    ui32WakeTime = xTaskGetTickCount();

    //
    // Loop forever.
    //
    while(1)
    {

       temperature=I2C_read16BitRegister(TMP101_ADDRESS,TMP101_TEMP_REG);
       temperature_conv=temperature/256; // conversion binary * 0.0625 (binary/16) /16 again

       xQueueSend(g_pI2cTempQueue, &temperature_conv, 100 / portTICK_RATE_MS);

        vTaskDelayUntil(&ui32WakeTime, 1000 / portTICK_RATE_MS);

    }
}

uint32_t
I2cTempTaskInit(void)
{

    // Configuration of keyboard
    //GPIO_Keypad_Init();


    // I2C init
       InitI2C0();
       SysCtlDelay(2000);
       I2C_write8BitRegister(TMP101_ADDRESS,TMP101_config_REG,command_tmp101);

    //
    // Create the LED task.
    //
    if(xTaskCreate(I2cTempTask, (const portCHAR *)"I2c Temperature", TEMPSTACKSIZE, NULL,
                   tskIDLE_PRIORITY + PRIORITY_TEMP_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //
    // Success.
    //
    return(0);
}
