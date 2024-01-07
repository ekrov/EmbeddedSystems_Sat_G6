/*
 * lcd_task.c
 *
 *  Created on: 21/12/2023
 *      Author: carlos almeida (only author)
 */


#include "lcd_task.h"

// Queue handles
// Keypad task will be read in this task, it is written by keypad task as it contains the latest keys written
QueueHandle_t g_pKeypadQueue;
// I2c temperature containing the latest temperature readings
QueueHandle_t g_pI2cTempQueue;
// Queue that stores the packets obtained in UART
QueueHandle_t uart_queue;
// Queue that stores the number of packets received
QueueHandle_t uart_queue_counter;
// Queue that stores the time since the time is initiated, in seconds
QueueHandle_t xTimerQueue;

// Semaphore handle for the buzzer, extern because it is used in several other tasks
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

// Main task of the LCD
static void LCDTask(void *pvParameters)
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
    uint8_t j;
    BaseType_t sucessfulReceived;

    float packet_number_idk;
    char buffer[buffer_size];
    char buffer_all_matrix[20][buffer_size];
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


    int16_t hour_received=0;
    int16_t minute_received=0;
    int16_t second_received=0;

    int8_t setup_time=0;
    int8_t press_init_key=0;

    int8_t setup_date=1;
    int8_t flag_receiving_packet=0;
    initiated=0;

    //
    // Get the current tick count.
    //
    ui32WakeTime = xTaskGetTickCount();

    //
    // Loop forever.
    //
    while(1)
    {
        // This not initiated section, represents the normal functioning of the system
        // after it is initiated
        if (initiated!=0){
            // If a key is received, its command is started
            if(xQueueReceive(g_pKeypadQueue, &TestKey, 1000) == pdPASS)
            {
                // All commands show first the key pressed
                if (TestKey>0 && TestKey!='A'){
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
                }

                // Each key has its corresponding command
                switch(TestKey) {
                // The command for key press 1 is showing the temperature, by peeking into the temperature queue
                  case 1:
                      temperature_display();
                    break;

                    // The command for key press 2 is unlocking the buzzer task, by giving the semaphore it is trying to take
                  case 2:
                      xSemaphoreGive(g_BuzzerSemaphore);
                      vTaskDelay( 1000 / portTICK_RATE_MS);
                    break;
                  // The command for key press 3
                  // This command displays the packet on the first position of the packet queue
                  case 3:

                      //Receives the packet in the first position of the queue
                      sucessfulReceived = xQueueReceive(uart_queue, &buffer, 1000);

                      // If a packet was read, displays the packet and shifts the LCD until it is all shown
                      if (sucessfulReceived == pdTRUE)
                      {
                          taskENTER_CRITICAL();
                          Lcd_Clear();
                          Lcd_Write_String("Packet:");
                          for (i = 4; i < (buffer_size-10); i++){
                               Lcd_Write_Char(buffer[i]);
                          }
                          taskEXIT_CRITICAL();
                          vTaskDelay( 1000 / portTICK_RATE_MS);
                          for (i = 4; i < (buffer_size-28); i++){
                              vTaskDelay( 300 / portTICK_RATE_MS);
                              Lcd_Shift_Left();
                          }
                      }
                      // If no packet was read from the queue, shows a error message
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
                    // The command for the key press 4
                    // Displays the number of received packets
                    // This reads from the uart queue counter
                    // that is incremented in the uart task
                  case 4:
                      xQueueReceive(uart_queue_counter, &uart_counter, 100);
                      taskENTER_CRITICAL();
                      Lcd_Clear();
                      Lcd_Write_String("Received pckts:");
                      if (uart_counter>0){
                          sprintf(snum, "%d", uart_counter);
                      }
                      for (i = 0; i < 5; i++)
                      {
                          snum_int=snum[i]-'0';
                          if (snum_int<=9 && snum_int>=0){
                            Lcd_Write_Char(snum[i]);
                          }

                      }
                      taskEXIT_CRITICAL();

                      vTaskDelay( 1000 / portTICK_RATE_MS);

                      break;
                  // The command for the key press 5
                  // Displays the current time
                  case 5:

                      if(xQueuePeek(xTimerQueue, &counter, 100) == pdPASS)
                        {
                          if (counter>3600)
                              hour_received=(counter/3600);
                          else
                              hour_received=0;
                          if (counter>(60))
                              minute_received=(counter-hour_received*3600)/60;
                          else
                              minute_received=0;
                          if (counter>0)
                              second_received=(counter-minute_received*60-hour_received*3600);
                          else
                              second_received=0;

                          date.hour=hour_received+date.starting_hour;
                          date.minute=minute_received+date.starting_minute;
                          date.second=second_received+date.starting_second;

                        taskENTER_CRITICAL();
                        Lcd_Clear();
                        Lcd_Write_String("Time:");

                          sprintf(time_counter, "%d", date.hour);
                          for (i = 0; i < 5; i++)
                          {
                              time_counter_int=time_counter[i]-'0';
                              if (time_counter_int<=9 && time_counter_int>=0){
                                Lcd_Write_Char(time_counter[i]);
                              }

                          }
                          Lcd_Write_String("h ");

                          sprintf(time_counter, "%d", date.minute);
                            for (i = 0; i < 5; i++)
                            {
                                time_counter_int=time_counter[i]-'0';
                                if (time_counter_int<=9 && time_counter_int>=0){
                                  Lcd_Write_Char(time_counter[i]);
                                }

                            }
                            Lcd_Write_String("m ");

                            sprintf(time_counter, "%d", date.second);
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
                  // The command for key 6
                  // Searches for a packet with the packet number inserted
                  // This command gets the 20 packets inserted into the queue and puts it in a matrix
                  // Then, the verifications of the packet numbers are done on the matrix itself, this is so no packet is
                  // inserted into the matrix while we are checking it
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

                                                           for (j=0;j<50;j++){
                                                               buffer_all_matrix[i][j]=buffer[j];
                                                           }

                                                           // re-send the received buffer to queue to the back
                                                          xQueueSendToBack(uart_queue, &buffer, portMAX_DELAY);
                                                      }
                                                      for (i = 0; i < 20; i++){
                                                          for (j=0;j<50;j++){
                                                              buffer[j]=buffer_all_matrix[i][j];
                                                          }
                                                           if (sucessfulReceived == pdTRUE)
                                                           {
                                                               // if packet numbers == keys pressed
                                                               packet_temp=buffer[5]-'0';
                                                               if (packet_temp==keys_Packet[0] || buffer[5]==32){

                                                                   packet_temp=buffer[6]-'0';
                                                                   if (packet_temp==keys_Packet[1] || buffer[6]==32){

                                                                       packet_temp=buffer[7]-'0';

                                                                       if (packet_temp==keys_Packet[2] || buffer[7]==32){

                                                                           packet_temp=buffer[8]-'0';

                                                                           if (packet_temp==TestKey && flag_receiving_packet==1){
                                                                               vTaskDelay( 1 / portTICK_RATE_MS);
                                                                               //show packet on lcd
                                                                               taskENTER_CRITICAL();
                                                                                 Lcd_Clear();
                                                                                 Lcd_Write_String("Packet:");
                                                                                 for (i = 4; i < (buffer_size-10); i++){
                                                                                      Lcd_Write_Char(buffer[i]);
                                                                                 }
                                                                                 taskEXIT_CRITICAL();
                                                                                 vTaskDelay( 1000 / portTICK_RATE_MS);
                                                                                 for (i = 4; i < (buffer_size-28); i++){
                                                                                     vTaskDelay( 300 / portTICK_RATE_MS);
                                                                                     Lcd_Shift_Left();
                                                                                 }

                                                                              vTaskDelay( 1000 / portTICK_RATE_MS);
                                                                              flag_receiving_packet=0;

                                                                           }
                                                                       }
                                                                   }
                                                               }

                                                              // If we still havent found the packet, displays the position we searched
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
                                                                   vTaskDelay( 300 / portTICK_RATE_MS);
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


                      break;
                  // The command for the key press 7 is similar to key 6
                  // The difference is that it only shows the RSSI and the SNR instead of the whole packet
                  case 7:
                      taskENTER_CRITICAL();
                        Lcd_Clear();
                        Lcd_Write_String("(RSSI) Pckt:");
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

                                                             for (j=0;j<50;j++){
                                                                 buffer_all_matrix[i][j]=buffer[j];
                                                             }

                                                             // re-send the received buffer to queue to the back
                                                            xQueueSendToBack(uart_queue, &buffer, portMAX_DELAY);
                                                        }
                                                        for (i = 0; i < 20; i++){
                                                            for (j=0;j<50;j++){
                                                                buffer[j]=buffer_all_matrix[i][j];
                                                            }
                                                             if (sucessfulReceived == pdTRUE)
                                                             {
                                                                 // if packet numbers == keys pressed
                                                                packet_temp=buffer[5]-'0';
                                                                if (packet_temp==keys_Packet[0] || buffer[5]==32){

                                                                    packet_temp=buffer[6]-'0';
                                                                    if (packet_temp==keys_Packet[1] || buffer[6]==32){

                                                                        packet_temp=buffer[7]-'0';

                                                                        if (packet_temp==keys_Packet[2] || buffer[7]==32){

                                                                            packet_temp=buffer[8]-'0';

                                                                             if (packet_temp==TestKey && flag_receiving_packet==1){
                                                                                 vTaskDelay( 1 / portTICK_RATE_MS);
                                                                                 //show packet on lcd
                                                                                taskENTER_CRITICAL();
                                                                                Lcd_Clear();
                                                                                //for (i = 0; i < 20; i++)
                                                                                //    Lcd_Write_Char(buffer[i]);
                                                                                Lcd_Write_String("Packet nr :");
                                                                                for (i = 4; i < 9; i++)
                                                                                 Lcd_Write_Char(buffer[i]);
                                                                                Lcd_Write_Char(':');
                                                                                for (i = 39; i < 44; i++)
                                                                                 Lcd_Write_Char(buffer[i]);

                                                                                taskEXIT_CRITICAL();

                                                                                vTaskDelay( 1000 / portTICK_RATE_MS);
                                                                                 for (i = 4; i < (buffer_size-40); i++){
                                                                                     vTaskDelay( 300 / portTICK_RATE_MS);
                                                                                     Lcd_Shift_Left();
                                                                                 }

                                                                              vTaskDelay( 1000 / portTICK_RATE_MS);
                                                                                flag_receiving_packet=0;
                                                                                /*idk=0;
                                                                              xQueueSend(g_pKeypadQueue, &idk, 100 / portTICK_RATE_MS);*/
                                                                             }
                                                                         }
                                                                     }
                                                                 }

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
                                                                     vTaskDelay( 300 / portTICK_RATE_MS);
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



                        break;
                  default:
                      vTaskDelay( 1 / portTICK_RATE_MS);

                }
                // This is the default screen of the system
                // It shows the time and the temperature
            }else{
                if(xQueuePeek(xTimerQueue, &counter, 100) == pdPASS)
                {
                Lcd_Write_Char('*');
                if (counter>3600)
                      hour_received=(counter/3600);
                  else
                      hour_received=0;
                  if (counter>(60))
                      minute_received=(counter-hour_received*3600)/60;
                  else
                      minute_received=0;
                  if (counter>0)
                      second_received=(counter-minute_received*60-hour_received*3600);
                  else
                      second_received=0;

                  date.hour=hour_received+date.starting_hour;
                  date.minute=minute_received+date.starting_minute;
                  date.second=second_received+date.starting_second;

                  if (date.second>60)
                  {
                      date.second=date.second-60;
                      date.minute=date.minute+1;
                  }
                taskENTER_CRITICAL();
                Lcd_Clear();
                //Lcd_Write_String("Time:");

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

                temperature_display();

                vTaskDelay( 100 / portTICK_RATE_MS);
                }
            }

        // This is the initial setup of the systems date, hour and where it waits for the start key
        }else{


            if (setup_date==1){
                // Start the system, by defining the date
                if (TestKey==200){
                    if (count_k_pressed<1 ){
                        taskENTER_CRITICAL();
                        Lcd_Clear();
                        Lcd_Write_String("Insert: MM--DD--YYYY");
                        taskEXIT_CRITICAL();
                        TestKey=201;
                    }
                }
                if (count_k_pressed<12){
                    Lcd_Set_Cursor(1,(9+count_k_pressed));
                }

                // This section counts the number of keys pressed
                // if one key was pressed, the inserted key was for the tens of the month
                // if a second key was pressed, it represents the units of the month, and so on
                // The TestKey variable is simply controling how many times the LCD is updated
                if(xQueueReceive(g_pKeypadQueue, &TestKey, 1) == pdPASS)
                {
                    if (TestKey<200){

                        if (TestKey<10 && count_k_pressed<12){
                            taskENTER_CRITICAL();
                             Lcd_Write_Char(TestKey + '0');
                             taskEXIT_CRITICAL();
                             count_k_pressed=count_k_pressed+1;

                             if (count_k_pressed==1){
                                 if (TestKey<2)
                                     date.month=TestKey*10;
                                 else
                                     count_k_pressed=count_k_pressed-1;
                             }
                             if (count_k_pressed==2){
                                 if (date.month==10 && TestKey<3)
                                     date.month=date.month+TestKey;
                                 else{
                                     if (date.month<10)
                                         date.month=date.month+TestKey;
                                     else
                                         count_k_pressed=count_k_pressed-1;
                                 }
                             }
                             if (count_k_pressed==5)
                             {
                                     if (TestKey<4)
                                         date.day=TestKey*10;
                                     else{
                                         count_k_pressed=count_k_pressed-1;
                                     }

                             }
                             if (count_k_pressed==6){
                                 if(date.day==30 && TestKey<2)
                                     date.day=date.day+TestKey;
                                 else
                                 {
                                     if (date.day<30)
                                         date.day=date.day+TestKey;
                                     else
                                         count_k_pressed=count_k_pressed-1;
                                 }
                             }
                             if (count_k_pressed==9)
                                     date.year=TestKey*1000;
                             if (count_k_pressed==10)
                                     date.year=date.year+TestKey*100;
                             if (count_k_pressed==11)
                                      date.year=date.year+TestKey*10;
                             if (count_k_pressed==12)
                                      date.year=date.year+TestKey;
                         }

                        vTaskDelay( 20 / portTICK_RATE_MS);
                    }
                }
                if (count_k_pressed==2)
                    count_k_pressed=4;
                if (count_k_pressed==6)
                    count_k_pressed=8;
                if (count_k_pressed==12)
                {
                    // If all keys were inserted, the setup goes to the next step, the time definition
                    setup_date=0;
                    setup_time=1;
                    count_k_pressed=0;
                    TestKey=200;

                }
            }
            // Definition of time setup
            if(setup_time==1){
                // Start the system, by defining the time
                if (TestKey==200){
                    if (count_k_pressed<1 ){
                        taskENTER_CRITICAL();
                        Lcd_Clear();
                        Lcd_Write_String("Insert: HH--MM--SS");
                        taskEXIT_CRITICAL();
                        TestKey=201;

                    }
                }
                if (count_k_pressed<12){
                    Lcd_Set_Cursor(1,(9+count_k_pressed));
                }

                // This section, similarly for the date, defines the time
                // This is based on the number of keys pressed
                if(xQueueReceive(g_pKeypadQueue, &TestKey, 1) == pdPASS)
                {
                    if (TestKey<200){

                        if (TestKey<10 && count_k_pressed<10){
                            taskENTER_CRITICAL();
                             Lcd_Write_Char(TestKey + '0');
                             taskEXIT_CRITICAL();
                             count_k_pressed=count_k_pressed+1;

                             if (count_k_pressed==1){
                                 if (TestKey<3)
                                     date.starting_hour=TestKey*10;
                                 else
                                     count_k_pressed=count_k_pressed-1;
                             }

                             if (count_k_pressed==2){
                                 if (date.starting_hour==20 && TestKey<5)
                                     date.starting_hour=date.starting_hour+TestKey;
                                 else{

                                     if(date.starting_hour<20)
                                         date.starting_hour=date.starting_hour+TestKey;
                                     else
                                         count_k_pressed=count_k_pressed-1;
                                 }
                             }
                             if (count_k_pressed==5){
                                 if (TestKey<6)
                                     date.starting_minute=TestKey*10;
                                 else
                                     count_k_pressed=count_k_pressed-1;
                             }
                              if (count_k_pressed==6){
                                     date.starting_minute=date.starting_minute+TestKey;
                              }
                             if (count_k_pressed==9){
                                 if (TestKey<6)
                                     date.starting_second=TestKey*10;
                                 else
                                     count_k_pressed=count_k_pressed-1;
                             }
                             if (count_k_pressed==10)
                                     date.starting_second=date.starting_second+TestKey;
                         }

                        vTaskDelay( 20 / portTICK_RATE_MS);
                    }
                }
                if (count_k_pressed==2)
                    count_k_pressed=4;
                if (count_k_pressed==6)
                    count_k_pressed=8;
                if (count_k_pressed==10)
                {
                    // The next and final step of the setup is queued
                    // Where it waits for the initial key press
                    setup_time=0;
                    press_init_key=1;
                    TestKey=200;
                    count_k_pressed=0;
                }
            }
            // If the initial key is pressed, the system is initiated with the "initiated" control variable
            if (press_init_key==1){
                if (TestKey==200){
                        taskENTER_CRITICAL();
                        Lcd_Clear();
                        Lcd_Write_String("Press A to start");
                        taskEXIT_CRITICAL();
                        TestKey=201;

                }
                if(xQueueReceive(g_pKeypadQueue, &TestKey, 1) == pdPASS)
                {
                    if (TestKey=='A')
                    {
                        initiated=1;
                    }
                }
            }

        }

        vTaskDelayUntil(&ui32WakeTime, 100 / portTICK_RATE_MS);
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

   // The keypad queue is created, with 1 slots, storing the latest key pressed
   g_pKeypadQueue = xQueueCreate(KEYPAD_QUEUE_SIZE, KEYPAD_ITEM_SIZE);
   // The temperature queue is created, storing the latest value, this is only accessed with peeks
   g_pI2cTempQueue = xQueueCreate(I2CTEMP_QUEUE_SIZE, I2CTEMP_ITEM_SIZE);
   // Initialization of the uart_queue that stores 20 packets
   uart_queue = xQueueCreate(UART_QUEUE_LENGTH, UART_SIZE);
   // Initialization of the queue that counts how many packets are obtained
   uart_queue_counter = xQueueCreate(UART_QUEUE_counter_LENGTH, UART_counter_SIZE);

   // -- TIMER --

   // Create the timer queue
   xTimerQueue = xQueueCreate(1, sizeof(int32_t));

   // Configure Timer0
   Timer0A_Init();

   // This testKey is simply controlling how many times the LCD is updated
   TestKey=200;
   xQueueSend(g_pKeypadQueue, &TestKey, 100 / portTICK_RATE_MS);

   // The uart counter queue is initiated with 0 packets received
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

void temperature_display(void){

    float Temperaturei2c;
    float temp2=0;
    float temp1=0;
    float temp0=0;

    if(xQueuePeek(g_pI2cTempQueue, &Temperaturei2c, 100) == pdPASS)
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

}
