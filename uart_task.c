/*
 * uart_task.c
 *
 *  Created on: 22/12/2023
 *      Author: carlo
 */

#include "uart_task.h"
QueueHandle_t uart_queue;
QueueHandle_t uart_queue_counter;

int32_t counter_uart=0;
void UART_Init(void)
{

    //
    // Enable the peripherals used by this example.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //
    // Set GPIO B0 and B1 as UART pins.
    //
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);

    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 9600, 8-N-1 operation.
    //
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 9600,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

}

int32_t UART_receive(void){

    int32_t uart_received;
    uart_received=UARTCharGetNonBlocking(UART1_BASE);
    return uart_received;



}

static void
UartTaskReceive(void *pvParameters)
{
    uint8_t i = 0;
        char receivedChar;
        char buffer[buffer_size];
        int charP = 0;
        int16_t uart_counter=0;

        while (1)
        {
            while (i < buffer_size - 1)
            {
                //Aguarda ate que um caracter esteja disponivel para leitura
                while (!UARTCharsAvail(UART1_BASE))
                    vTaskDelay(pdMS_TO_TICKS(1));

                // Le um caractere da porta UART
                receivedChar = UARTCharGet(UART1_BASE);

                // Verifica se e um caracter de terminaçao
                if (receivedChar == 'P')
                {
                    charP = 1;
                }

                // Verifica se e um caracter de terminaçao
                if (receivedChar == '\n' || receivedChar == '\r')
                {
                    charP = 0;
                    // Adiciona o caracter nulo ao final da string
                    buffer[i] = '\0';
                    xQueueSend(uart_queue, &buffer, portMAX_DELAY);
                    //xQueueReceive(uart_queue_counter, &uart_counter, 100);
                    uart_counter=uart_counter+1;
                    xQueueOverwrite(uart_queue_counter, &uart_counter);

                    i = 0;
                    break;
                }

                // Armazena o caracter no buffer
                if (charP == 1)
                {
                    buffer[i++] = receivedChar;
                    //Lcd_Write_Char(receivedChar);
                    //i++;
                    //return buffer;
                }
            }


            vTaskDelay(pdMS_TO_TICKS(10));
        }
}

uint32_t
UartTaskReceiveInit(void)
{

    // UART
    UART_Init();

    //
    // Create the Uart task.
    //
    if(xTaskCreate(UartTaskReceive, (const portCHAR *)"Uart receive", UARTSTACKSIZE, NULL,
                   tskIDLE_PRIORITY + PRIORITY_UART_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //
    // Success.
    //
    return(0);
}

