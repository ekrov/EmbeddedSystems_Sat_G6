/*
 * lcd_task.c
 *
 *  Created on: 21/12/2023
 *      Author: carlos almeida (only author)
 */


#include "lcd_task.h"
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

// Queue handles
xQueueHandle g_pKeypadQueue;
xQueueHandle g_pI2cTempQueue;
QueueHandle_t uart_queue;
QueueHandle_t uart_queue_counter;
QueueHandle_t xTimerQueue;

extern xSemaphoreHandle g_BuzzerSemaphore;

int32_t packet_temp;
void Lcd_Port(char a)
{
if(a & 1)
    GPIOPinWrite(GPIO_PORTC_BASE, D4, D4);
else
    GPIOPinWrite(GPIO_PORTC_BASE, D4, 0);
if(a & 2)
    GPIOPinWrite(GPIO_PORTC_BASE, D5, D5);
else
    GPIOPinWrite(GPIO_PORTC_BASE, D5, 0);
if(a & 4)
    GPIOPinWrite(GPIO_PORTC_BASE, D6, D6);
else
    GPIOPinWrite(GPIO_PORTC_BASE, D6, 0);
if(a & 8)
    GPIOPinWrite(GPIO_PORTC_BASE, D7, D7);
else
    GPIOPinWrite(GPIO_PORTC_BASE, D7, 0);
}
/**************************************************************
* Function: void Lcd_Cmd (char a)
*
* Returns: Nothing
*
* Description: Sets LCD command
**************************************************************/
void Lcd_Cmd(char a)
{
    GPIOPinWrite(GPIO_PORTF_BASE, RS, 0); // => RS = 0
    Lcd_Port(a);

    GPIOPinWrite(GPIO_PORTF_BASE, EN, EN); // => E = 1
    SysCtlDelay(26667);

    GPIOPinWrite(GPIO_PORTF_BASE, EN, 0); // => E = 0
    SysCtlDelay(26667);
}
/**************************************************************
* Function: void Lcd_Clear()
*
* Returns: Nothing
*
* Description: Clears the LCD
**************************************************************/
void Lcd_Clear(void)
{
Lcd_Cmd(0);
Lcd_Cmd(1);
}
/**************************************************************
* Function: void Lcd_Set_Cursor(char a, char b)
*
* Returns: Nothing
*
* Description: Sets the LCD cursor position
**************************************************************/
void Lcd_Set_Cursor(char a, char b)
{
char temp,z,y;
if(a == 1)
{
temp = 0x80 + b - 1;
z = temp>>4;
y = temp & 0x0F;
Lcd_Cmd(z);
Lcd_Cmd(y);
}

else if(a == 2)
{
temp = 0xC0 + b - 1;
z = temp>>4;
y = temp & 0x0F;
Lcd_Cmd(z);
Lcd_Cmd(y);
}
}
/**************************************************************
* Function: void Lcd_Init()
*
* Returns: Nothing
*
* Description: Initializes the LCD
**************************************************************/
void Lcd_Init(void)
{
    Lcd_Port(0x00);
    //__delay_ms(40);
    SysCtlDelay(10000);
    Lcd_Cmd(0x03);
    SysCtlDelay(10000);
    Lcd_Cmd(0x03);
    SysCtlDelay(10000);
    Lcd_Cmd(0x03);
    /////////////////////////////////////////////////////
    Lcd_Cmd(0x02);
    Lcd_Cmd(0x02);//Function set 1, 0-4bits
    Lcd_Cmd(0x00);// nº linhas  font 5x8 Nº de linhas 1

    Lcd_Cmd(0x00);// display on/off
    Lcd_Cmd(0x0F);// 1, Display-on, Cursor - 1, Blink -0


    Lcd_Cmd(0x00);// entry mode set
    Lcd_Cmd(0x06);// increment the address by 1, shift the cursor to right

}
/**************************************************************
* Function: void Lcd_Write_Char (char a)
*
* Returns: Nothing
*
* Description: Writes a character to the LCD
**************************************************************/
void Lcd_Write_Char(char a)
{
    char temp,y;
    temp = a&0x0F;
    y = a&0xF0;
    GPIOPinWrite(GPIO_PORTF_BASE, RS, RS); // => RS = 1
    Lcd_Port(y>>4); //Data transfer
    GPIOPinWrite(GPIO_PORTF_BASE, EN, EN); // => EN = 1
    SysCtlDelay(10000);
    GPIOPinWrite(GPIO_PORTF_BASE, EN, 0); // => EN = 0
    Lcd_Port(temp);
    GPIOPinWrite(GPIO_PORTF_BASE, EN, EN); // => EN = 1
    SysCtlDelay(4000);
    GPIOPinWrite(GPIO_PORTF_BASE, EN, 0); // => EN = 0
}

/**************************************************************
* Function: void Lcd_Write_String (const char *a)
*
* Returns: Nothing
*
* Description: Writes a string to the LCD
**************************************************************/
void Lcd_Write_String(const char *a)
{
int i;
for(i=0;a[i]!='\0';i++)
Lcd_Write_Char(a[i]);
}
/**************************************************************
* Function: void Lcd_Shift_Right()
*
* Returns: Nothing
*
* Description: Shifts text on the LCD right
**************************************************************/
void Lcd_Shift_Right(void)
{
Lcd_Cmd(0x01);
Lcd_Cmd(0x0C);
}

/**************************************************************
* Function: void Lcd_Shift_Left()
*
* Returns: Nothing
*
* Description: Shifts text on the LCD left
**************************************************************/
void Lcd_Shift_Left(void)
{
Lcd_Cmd(0x01);
Lcd_Cmd(0x08);
}
/**************************************************************
* Function: void Lcd_Write_Integer(int v)
*
* Returns: Nothing
*
* Description: Converts a string to an integer
**************************************************************/
void Lcd_Write_Integer(int v)
{
char buf[8];
Lcd_Write_String(itoa(buf, v, 10));
}

/**************************************************************
* Function: void Lcd_Write_Float(float f)
*
* Returns: Nothing
*
* Description: Converts a string to a float
**************************************************************/
void Lcd_Write_Float(float f)
{
char* buf11;
int status;
buf11 = ftoa(f, &status);
Lcd_Write_String(buf11);
}


static void
LCDTask(void *pvParameters)
{
    portTickType ui32WakeTime;
    uint32_t ui32LEDToggleDelay;
    uint8_t TestKey;
    int32_t keys_Packet[4];
    //int32_t packet_temp;
    uint8_t idk;
    int16_t uart_counter;
    int8_t snum_int=0;
    int8_t count_k_pressed=0;
    uint8_t i;
    BaseType_t sucessfulReceived;
    float Temperaturei2c;
    float temp2=0;
    float temp1=0;
    float temp0=0;
    float packet_number_idk;
    char buffer[50];
    int32_t counter = 0;
    uint32_t previous_counter = 0;
    char time_counter[5];
    int32_t time_counter_int=0;
    char snum[5];

    date.hour=0;
    date.minute=0;
    date.second=0;

    date.starting_hour=0;
    date.starting_minute=0;
    date.starting_second=0;


    int16_t hour_received;
    int16_t minute_received;
    int16_t second_received;

    int8_t initiated=0;
    int8_t setup_time=0;

    int8_t flag_receiving_packet=0;
    //
    // Initialize the LED Toggle Delay to default value.
    //
    //ui32LEDToggleDelay = LED_TOGGLE_DELAY;

    //
    // Get the current tick count.
    //
    ui32WakeTime = xTaskGetTickCount();

    //
    // Loop forever.
    //
    while(1)
    {
        if (initiated!=0){
            if(xQueueReceive(g_pKeypadQueue, &TestKey, 1000) == pdPASS)
            {
                if (TestKey>0){
                taskENTER_CRITICAL();

                 Lcd_Clear();
                 Lcd_Write_Char('K');
                 Lcd_Write_Char(':');
                 Lcd_Write_Char(' ');
                 if (TestKey<10){
                     Lcd_Write_Char(TestKey + '0');
                 }
                 else
                 {
                     Lcd_Write_Char(TestKey);
                 }
                taskEXIT_CRITICAL();

                idk=0;
               xQueueSend(g_pKeypadQueue, &idk, 100 / portTICK_RATE_MS);

                }


                switch(TestKey) {
                  case 1:
                      if(xQueueReceive(g_pI2cTempQueue, &Temperaturei2c, 100) == pdPASS)
                      {
                          taskENTER_CRITICAL();
                          //dezenas
                         temp2=Temperaturei2c/10;

                         // unidades
                         temp1=Temperaturei2c-((int) temp2*10);

                         //decimal
                         temp0= (int)temp1;
                         temp0=temp1-temp0;
                         temp0=temp0*10.0;
                         temp0=round(temp0);

                         Lcd_Write_Char(' ');
                         Lcd_Write_Char('T');
                         Lcd_Write_Char(':');
                         Lcd_Write_Char(temp2+'0');
                         Lcd_Write_Char(temp1+'0');
                         Lcd_Write_Char('.');
                         Lcd_Write_Char(temp0+'0');
                          taskEXIT_CRITICAL();


                          vTaskDelay( 1000 / portTICK_RATE_MS);


                      }

                    // code block
                    break;
                  case 2:
                    // code block
                      xSemaphoreGive(g_BuzzerSemaphore);
                      vTaskDelay( 1000 / portTICK_RATE_MS);

                    break;
                  case 3:

                      //Recebe dados da fila
                      sucessfulReceived = xQueueReceive(uart_queue, &buffer, 100);

                      if (sucessfulReceived == pdTRUE)
                      {
                          //packet_number_idk=packet_division(&buffer, 1);
                          taskENTER_CRITICAL();
                          Lcd_Clear();
                          //for (i = 0; i < 20; i++)
                          //    Lcd_Write_Char(buffer[i]);
                          //Lcd_Write_String("Packet nr :");

//                          Lcd_Write_Char('P');
//                          Lcd_Write_Char('a');
//                          Lcd_Write_Char('c');
//                          Lcd_Write_Char('k');
//                          Lcd_Write_Char('e');
//                          Lcd_Write_Char('t');
//                          Lcd_Write_Char(' ');

                          Lcd_Write_String("Packet:");
                          for (i = 4; i < 8; i++)
                               Lcd_Write_Char(buffer[i]);
                          //Lcd_Write_Float(packet_number);
                          //Lcd_Write_Char(packet_number_idk+'0');

                          taskEXIT_CRITICAL();
                        //vTaskDelay( 1 / portTICK_RATE_MS);


                      }
                      else
                      {
                          taskENTER_CRITICAL();
                          Lcd_Clear();
                          Lcd_Write_Char('N');
                          Lcd_Write_Char('O');
                          Lcd_Write_Char(' ');
                          Lcd_Write_Char('D');
                          Lcd_Write_Char('A');
                          Lcd_Write_Char('T');
                          Lcd_Write_Char('A');

                          taskEXIT_CRITICAL();


                      }
                      vTaskDelay( 1000 / portTICK_RATE_MS);



                    break;
                  case 4:
                      xQueueReceive(uart_queue_counter, &uart_counter, 100);
                      taskENTER_CRITICAL();
                      Lcd_Clear();
                      Lcd_Write_String("Received pckts:");
                      // Convert 123 to string [buf]
                      if (uart_counter>0){
                          sprintf(snum, "%d", uart_counter);
                      }
                      //itoa(uart_counter, snum, 10);
                      for (i = 0; i < 5; i++)
                      {
                          snum_int=snum[i]-'0';
                          if (snum_int<=9 && snum_int>=0){
                            Lcd_Write_Char(snum[i]);
                          }

                      }
                      //Lcd_Write_String("        ");
                      taskEXIT_CRITICAL();

                      vTaskDelay( 1000 / portTICK_RATE_MS);

                      break;

                  case 5:

                      if(xQueuePeek(xTimerQueue, &counter, 100) == pdPASS)
                        {

                          hour_received=(counter/3600);
                          minute_received=(counter-hour_received*3600)/60;
                          second_received=(counter-minute_received*60-hour_received*3600);

                          date.hour=hour_received+date.starting_hour;
                          date.minute=minute_received+date.starting_hour;
                          date.second=second_received+date.starting_second;

                        taskENTER_CRITICAL();
                        Lcd_Clear();
                        Lcd_Write_String("Time:");

                          sprintf(time_counter, "%d", date.hour);
                          //itoa(uart_counter, snum, 10);
                          for (i = 0; i < 5; i++)
                          {
                              time_counter_int=time_counter[i]-'0';
                              if (time_counter_int<=9 && time_counter_int>=0){
                                Lcd_Write_Char(time_counter[i]);
                              }

                          }
                          Lcd_Write_String("h ");

                          sprintf(time_counter, "%d", date.minute);
                            //itoa(uart_counter, snum, 10);
                            for (i = 0; i < 5; i++)
                            {
                                time_counter_int=time_counter[i]-'0';
                                if (time_counter_int<=9 && time_counter_int>=0){
                                  Lcd_Write_Char(time_counter[i]);
                                }

                            }
                            Lcd_Write_String("m ");

                            sprintf(time_counter, "%d", date.second);
                            //itoa(uart_counter, snum, 10);
                            for (i = 0; i < 5; i++)
                            {
                                time_counter_int=time_counter[i]-'0';
                                if (time_counter_int<=9 && time_counter_int>=0){
                                  Lcd_Write_Char(time_counter[i]);
                                }

                            }
                            Lcd_Write_Char('s');
                        taskEXIT_CRITICAL();


                        vTaskDelay( 1000 / portTICK_RATE_MS);
                        }

                      break;

                  case 6:
                      taskENTER_CRITICAL();
                      Lcd_Clear();
                      Lcd_Write_String("Pckt: ");
                      taskEXIT_CRITICAL();
                      flag_receiving_packet=1;

                      // Initially a 0 is sent, this safeguards that
                      if(xQueueReceive(g_pKeypadQueue, &TestKey, (5000 / portTICK_RATE_MS)) == pdPASS){
                          vTaskDelay( 2 / portTICK_RATE_MS);
                      }
                      // Selecionar packet a observar
                      while(flag_receiving_packet==1){
                          if(xQueueReceive(g_pKeypadQueue, &TestKey, (5000 / portTICK_RATE_MS)) == pdPASS){
                                  keys_Packet[0]=TestKey;
                                  taskENTER_CRITICAL();
                                  Lcd_Write_Char(TestKey+'0');
                                  taskEXIT_CRITICAL();
                                  if(xQueueReceive(g_pKeypadQueue, &TestKey, (5000 / portTICK_RATE_MS)) == pdPASS){
                                        keys_Packet[1]=TestKey;
                                        taskENTER_CRITICAL();
                                          Lcd_Write_Char(TestKey+'0');
                                          taskEXIT_CRITICAL();
                                        if(xQueueReceive(g_pKeypadQueue, &TestKey, (5000 / portTICK_RATE_MS)) == pdPASS){
                                            keys_Packet[2]=TestKey;
                                            taskENTER_CRITICAL();
                                              Lcd_Write_Char(TestKey+'0');
                                              taskEXIT_CRITICAL();
                                                if(xQueueReceive(g_pKeypadQueue, &TestKey, (5000 / portTICK_RATE_MS)) == pdPASS){
                                                        keys_Packet[3]=TestKey;
                                                        taskENTER_CRITICAL();
                                                      Lcd_Write_Char(TestKey+'0');
                                                      taskEXIT_CRITICAL();

                                                      for (i = 0; i < 20; i++){
                                                          //Recebe dados da fila
                                                           sucessfulReceived = xQueueReceive(uart_queue, &buffer, 100);

                                                           if (sucessfulReceived == pdTRUE)
                                                           {
                                                               // if packet numbers == keys pressed
                                                               packet_temp=buffer[4]-'0';
                                                               if (packet_temp==keys_Packet[0] || buffer[4]==32){

                                                                   packet_temp=buffer[5]-'0';
                                                                   if (packet_temp==keys_Packet[1] || buffer[5]==32){

                                                                       packet_temp=buffer[6]-'0';

                                                                       if (packet_temp==keys_Packet[2] || buffer[6]==32){

                                                                           packet_temp=buffer[7]-'0';

                                                                           if (packet_temp==TestKey){
                                                                               vTaskDelay( 1 / portTICK_RATE_MS);
                                                                               //show packet on lcd
                                                                              taskENTER_CRITICAL();
                                                                              Lcd_Clear();
                                                                              //for (i = 0; i < 20; i++)
                                                                              //    Lcd_Write_Char(buffer[i]);
                                                                              Lcd_Write_String("Packet nr :");
                                                                              for (i = 4; i < 12; i++)
                                                                               Lcd_Write_Char(buffer[i]);
                                                                              //Lcd_Write_Float(packet_number);
                                                                              //Lcd_Write_Char(packet_number_idk+'0');

                                                                              taskEXIT_CRITICAL();

                                                                              vTaskDelay( 1000 / portTICK_RATE_MS);
                                                                              flag_receiving_packet=0;
                                                                              idk=0;
                                                                            xQueueSend(g_pKeypadQueue, &idk, 100 / portTICK_RATE_MS);
                                                                           }
                                                                       }
                                                                   }
                                                               }
                                                               // if packet numbers != keys pressed

                                                               // re-send the received buffer to queue to the back
                                                               xQueueSendToBack(uart_queue, &buffer, portMAX_DELAY);
                                                               if (flag_receiving_packet==1){
                                                                   taskENTER_CRITICAL();
                                                                   Lcd_Clear();
                                                                   Lcd_Write_String("Didnt find in ");
                                                                   if (i<=9){
                                                                       Lcd_Write_Char(i+'0');}
                                                                   else{
                                                                       Lcd_Write_Char((i/10)+'0');
                                                                       Lcd_Write_Char((i-((i/10)*10))+'0');}

                                                                   taskEXIT_CRITICAL();
                                                                   vTaskDelay( 1000 / portTICK_RATE_MS);
                                                               }
                                                           }
                                                       }
                                                      flag_receiving_packet=0;

                                                      vTaskDelay( 1 / portTICK_RATE_MS);
                                        }
                                      }
                                  }
                          }
                      }

                       //vTaskDelay( 1000 / portTICK_RATE_MS);
                       idk=0;
                       xQueueSend(g_pKeypadQueue, &idk, 100 / portTICK_RATE_MS);

                      break;
                  case 0:
                      taskENTER_CRITICAL();
                     Lcd_Clear();
                     Lcd_Write_Char('-');
                     Lcd_Write_Char('-');
                     Lcd_Write_Char('-');
                     Lcd_Write_Char('-');
                     Lcd_Write_Char('P');
                     Lcd_Write_Char('R');
                     Lcd_Write_Char('E');
                     Lcd_Write_Char('S');
                     Lcd_Write_Char('S');
                     Lcd_Write_Char(' ');
                     Lcd_Write_Char('A');
                     Lcd_Write_Char(' ');
                     Lcd_Write_Char('K');
                     Lcd_Write_Char('E');
                     Lcd_Write_Char('Y');
                     Lcd_Write_Char('-');
                     Lcd_Write_Char('-');
                     Lcd_Write_Char('-');
                     Lcd_Write_Char('-');
                     taskEXIT_CRITICAL();
                      break;

                  default:
                      vTaskDelay( 1000 / portTICK_RATE_MS);

                    // code block
                }
            }
            TestKey=200;

        }else{

            if (setup_time==0){
                // Start the system, by defining the date and the time
                if (count_k_pressed<1 ){
                    taskENTER_CRITICAL();
                    Lcd_Clear();
                    Lcd_Write_String("Insert: MM--DD--YYYY");
                    taskEXIT_CRITICAL();
                }
                if (count_k_pressed<12){
                    Lcd_Set_Cursor(1,(9+count_k_pressed));
                }


                if(xQueueReceive(g_pKeypadQueue, &TestKey, 1) == pdPASS)
                {
                    if (TestKey<200){

                        if (TestKey<10 && count_k_pressed<12){
                            taskENTER_CRITICAL();
                             Lcd_Write_Char(TestKey + '0');
                             taskEXIT_CRITICAL();
                             count_k_pressed=count_k_pressed+1;

                             if (count_k_pressed==1)
                                     date.day=TestKey*10;
                             if (count_k_pressed==2)
                                     date.day=date.day+TestKey;
                             if (count_k_pressed==4)
                                     date.month=TestKey*10;
                             if (count_k_pressed==5)
                                     date.month=date.month+TestKey;
                             if (count_k_pressed==8)
                                     date.month=TestKey;
                             if (count_k_pressed==9)
                                     date.year=TestKey*10;
                         }

                        vTaskDelay( 100 / portTICK_RATE_MS);
                    }
                }
                if (count_k_pressed==2)
                    count_k_pressed=4;
                if (count_k_pressed==6)
                    count_k_pressed=8;
                if (count_k_pressed==12)
                {
                    setup_time=1;
                    count_k_pressed=0;

                    /*initiated=1;
                    idk=0;
                    xQueueSend(g_pKeypadQueue, &idk, 100 / portTICK_RATE_MS);*/
                }
            }else{
                // Start the system, by defining the date and the time
                if (count_k_pressed<1 ){
                    taskENTER_CRITICAL();
                    Lcd_Clear();
                    Lcd_Write_String("Insert: HH--MM--SS");
                    taskEXIT_CRITICAL();
                }
                if (count_k_pressed<12){
                    Lcd_Set_Cursor(1,(9+count_k_pressed));
                }


                if(xQueueReceive(g_pKeypadQueue, &TestKey, 1) == pdPASS)
                {
                    if (TestKey<200){

                        if (TestKey<10 && count_k_pressed<10){
                            taskENTER_CRITICAL();
                             Lcd_Write_Char(TestKey + '0');
                             taskEXIT_CRITICAL();
                             count_k_pressed=count_k_pressed+1;

                             if (count_k_pressed==1)
                                     date.starting_hour=TestKey*10;
                             if (count_k_pressed==2)
                                     date.starting_hour=date.starting_hour+TestKey;
                             if (count_k_pressed==4)
                                     date.starting_minute=TestKey*10;
                             if (count_k_pressed==5)
                                     date.starting_minute=date.starting_minute+TestKey;
                             if (count_k_pressed==8)
                                     date.starting_second=TestKey*10;
                             if (count_k_pressed==9)
                                     date.starting_second=date.starting_second+TestKey;
                         }

                        vTaskDelay( 100 / portTICK_RATE_MS);
                    }
                }
                if (count_k_pressed==2)
                    count_k_pressed=4;
                if (count_k_pressed==6)
                    count_k_pressed=8;
                if (count_k_pressed==10)
                {
                    initiated=1;
                    idk=0;
                    xQueueSend(g_pKeypadQueue, &idk, 100 / portTICK_RATE_MS);
                }
            }

        }

        vTaskDelayUntil(&ui32WakeTime, 400 / portTICK_RATE_MS);
    }
}

uint32_t
LCDTaskInit(void)
{

    uint8_t TestKey;
    // init LCD
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_5);
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_6);
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_7);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);

    // LCD start
    Lcd_Init();

    //LCD clear
   SysCtlDelay(2000);
   Lcd_Clear();

   g_pKeypadQueue = xQueueCreate(KEYPAD_QUEUE_SIZE, KEYPAD_ITEM_SIZE);
   g_pI2cTempQueue = xQueueCreate(I2CTEMP_QUEUE_SIZE, I2CTEMP_ITEM_SIZE);
   // Inicialização do uart_queue
   uart_queue = xQueueCreate(UART_QUEUE_LENGTH, UART_SIZE);
   uart_queue_counter = xQueueCreate(UART_QUEUE_counter_LENGTH, UART_counter_SIZE);

   // -- TIMER --

   // Create the timer queue
   xTimerQueue = xQueueCreate(1, sizeof(int32_t));

   // Configure Timer0
   Timer0A_Init();
   /*
   // Create the timer
   TimerHandle_t xTimer = xTimerCreate("HardwareTimer", pdMS_TO_TICKS(1000), pdTRUE, 0, vTimerCallback);

   // Start the timer
   xTimerStart(xTimer, portMAX_DELAY);
   */
   // --
   TestKey=200;
   xQueueSend(g_pKeypadQueue, &TestKey, 100 / portTICK_RATE_MS);

   xQueueOverwrite(uart_queue_counter, &TestKey);



   // Verificação temp_queue e buzzer_queue
   if (g_pKeypadQueue == NULL || g_pI2cTempQueue == NULL || uart_queue == NULL)
   {
       printf("Error creating queue");


   }

    //
    // Create the LCD task.
    //
    if(xTaskCreate(LCDTask, (const portCHAR *)"LCD", LCDTASKSTACKSIZE, NULL,tskIDLE_PRIORITY + PRIORITY_LCD_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //
    // Success.
    //
    return(0);
}
