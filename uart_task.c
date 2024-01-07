/*
 * uart_task.c
 *
 *  Created on: 22/12/2023
 *      Author: carlo
 */

#include "uart_task.h"
#include "lcd_task.h"
QueueHandle_t uart_queue;
QueueHandle_t uart_queue_counter;
extern xSemaphoreHandle g_uartSemaphore;
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

//int32_t UART_receive(void){
//
//    int32_t uart_received;
//    uart_received=UARTCharGetNonBlocking(UART1_BASE);
//    return uart_received;
//
//}

static void
UartTaskReceive(void *pvParameters)
{
        uint8_t i = 0;
        uint8_t cont = 0;

        char receivedChar;
        char buffer[buffer_size];
        char buffer_temp[buffer_size];
        int32_t temp;
        int charP = 0;
        int32_t uart_counter=0;
        BaseType_t queue_uart_status;
        int32_t spaces_queue;
        char charValue;
        char time_counter[2];
        initiated=0;
        while (1)
        {

            if ((xSemaphoreTake(g_uartSemaphore, portMAX_DELAY) == pdTRUE)){
                while (i < (buffer_size - 11))
                {
                    //Aguarda ate que um caracter esteja disponivel para leitura
                    while (!UARTCharsAvail(UART1_BASE))
                        vTaskDelay(pdMS_TO_TICKS(1));
                    // Le um caracter da porta UART
                    receivedChar = UARTCharGet(UART1_BASE);
                    // Verifica se o caracter recebido é o caracter inicial
                    if (receivedChar == 'P')
                    {
                        charP = 1;
                    }
                    // Verifica se o caracter recebido é o caracter final do packet
                    // E se for, envia o buffer preenchido para a queue
                    if ((receivedChar == '\n' || receivedChar == '\r') && charP!=0 )
                    {

                        if (date.second>0){
                            charP = 0;
                            buffer[i+1] = '\0';
                            buffer[i+2]='_';
                            // Hours
                            if (date.hour>0){
                                temp=(date.hour/10);
                                if (temp>0)
                                    charValue = temp + '0';
                                else
                                    charValue = '0';
                                buffer[i+3]=charValue;
                                temp=(date.hour-(date.hour/10)*10);
                                if (temp>0)
                                    charValue = temp + '0';
                                else
                                    charValue = '0';
                                buffer[i+4]=charValue;
                                buffer[i+5]='h';
                            }else{
                                buffer[i+3]='_';
                                buffer[i+4]='_';
                                buffer[i+5]='h';
                            }
                            if (date.minute>0){
                                // Minutes
                                temp=(date.minute/10);
                                if (temp>0)
                                    charValue = temp + '0';
                                else
                                    charValue = '0';
                                buffer[i+6]=charValue;
                                temp= (date.minute-(date.minute/10)*10);
                                if (temp>0)
                                    charValue = temp + '0';
                                else
                                    charValue = '0';
                                buffer[i+7]=charValue;
                                buffer[i+8]='m';
                            }else{
                                buffer[i+6]='_';
                                buffer[i+7]='_';
                                buffer[i+8]='m';
                            }
                            // Seconds
                            if (date.second>0){
                                temp= (date.second/10);
                                if (temp>0)
                                    charValue = temp + '0';
                                else
                                    charValue = '0';
                                buffer[i+9]=charValue;
                                temp=(date.second-(date.second/10)*10);
                                if (temp>0)
                                    charValue = temp + '0';
                                else
                                    charValue = '0';
                                buffer[i+10]=charValue;
                                buffer[i+11]='s';
                            }else{
                                buffer[i+9]='_';
                                buffer[i+10]='_';
                                buffer[i+11]='s';
                            }

                            spaces_queue=uxQueueSpacesAvailable(uart_queue);
                            if ( spaces_queue < 1){
                                xQueueReceive(uart_queue, &buffer_temp,portMAX_DELAY);
                                xQueueSendToBack(uart_queue, &buffer, portMAX_DELAY);
                            }else{
                                xQueueSendToBack(uart_queue, &buffer, portMAX_DELAY);
                            }
                            uart_counter=uart_counter+1;
                            xQueueOverwrite(uart_queue_counter, &uart_counter);
                            i = 0;
                    }
                        charP = 0;
                        i=0;
                        break;
                    }

                    // Armazena o caracter no buffer
                    if (charP == 1)
                    {
                        i++;
                        buffer[i] = receivedChar;
                    }
                }
            }
        }
}

//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void
UARTIntHandler(void)
{

    // Verifique a fonte da interrupção UART
        uint32_t ui32Status = UARTIntStatus(UART1_BASE, true);
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        // Limpe as interrupções UART
        UARTIntClear(UART1_BASE, ui32Status);

        xSemaphoreGiveFromISR(g_uartSemaphore,&xHigherPriorityTaskWoken);

        // Lógica de tratamento de interrupção UART

        // Indica que uma mudança de contexto pode ser realizada
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

uint32_t
UartTaskReceiveInit(void)
{
    /*
    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPUEnable();
    ROM_FPULazyStackingEnable();
    */



    // UART
    UART_Init();

    //
    // Enable processor interrupts.
    //
    IntMasterEnable();

    //
    // Enable the UART interrupt for UART1_BASE
    //
    IntEnable(INT_UART1);
    IntPrioritySet(INT_UART1, 0);
    UARTIntRegister(UART1_BASE, UARTIntHandler);
    UARTIntEnable(UART1_BASE, UART_INT_RX);

    //
    // Create the Uart task.
    //
    if(xTaskCreate(UartTaskReceive, (const portCHAR *)"Uart receive", UARTSTACKSIZE, NULL,tskIDLE_PRIORITY + PRIORITY_UART_TASK, NULL) != pdPASS)
    {
        return(1);
    }

    //
    // Success.
    //
    //vTaskDelay(pdMS_TO_TICKS(1));
    return(0);
}

