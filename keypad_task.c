/*
 * keypad_task.c
 *
 *  Created on: 21/12/2023
 *      Author: carlo
 */





#include "keypad_task.h"

const uint8_t Keypad[num_Rows][num_Cols]=  {  {1,2,3,'F'},
                                              {4,5,6,'E'},
                                              {7,8,9,'D'},
                                              {'A',0,'B','C'} };
uint8_t RowPins[num_Rows]={Row_0,Row_1,Row_2,Row_3}; //Depend on pin connections
uint8_t ColPins[num_Cols]={Col_0,Col_1,Col_2,Col_3}; //Depend on pin connections

//---------------Global variables------------//
uint8_t cur_Col,cur_Row,K_pressed=0;
uint32_t Input_value;
uint8_t Test;
uint8_t i;
uint8_t count=0;
uint8_t idk[4];
uint8_t temp;

xQueueHandle g_pKeypadQueue;


/*-----GPIO Initialization for keypad-----//
 * Rows         -Inputs (pulls-up) - GPIO portB pins 0-3
 * Columns      -Outputs           - GPIO portB pins 4-7
 */
void GPIO_Keypad_Init(){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeGPIOOutput(Cols_port, Cols);
    GPIOPinTypeGPIOInput(Rows_port, Rows);
    GPIOPadConfigSet(Rows_port, Rows, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);
}

/*-----Check Keypad if any key is pressed----//
 * Use input (rows) to check
 * If any row don't have value "1" --> at least one key pressed
 * retun 1
*/
uint8_t Check_KPad(){
    GPIOPinWrite(Cols_port, Cols, 0);
    idk[0]=GPIOPinRead(GPIO_PORTE_BASE,Row_0);
    //SysCtlDelay(SysCtlClockGet()/30);
    vTaskDelay( 10);
    idk[1]=GPIOPinRead(GPIO_PORTE_BASE,Row_1);
    vTaskDelay( 10);

    //SysCtlDelay(SysCtlClockGet()/30);
    idk[2]=GPIOPinRead(GPIO_PORTE_BASE,Row_2);
    vTaskDelay( 10);

    //SysCtlDelay(SysCtlClockGet()/30);
    idk[3]=GPIOPinRead(GPIO_PORTE_BASE,Row_3);
    //SysCtlDelay(SysCtlClockGet()/30);
    vTaskDelay( 10);

    for(i=0;i<4;i++){
        if (idk[i]==0){
            return 1;
        }
    }
    return 0;
    /*if((Input_value&0x0F)!=0x0F)
        return 1;
    else
        return 0;*/
}

/*Get the key are pressed in the  keyboard if detect at least one key is pressed//
 *because all inputs pulls-up so they have value "0" if button is pressed
 * -First set all outputs pin to 1, then clear col 0 (other have "1") to 0 to check
 *   if pressed button in col 0 by collects all inputs from rows ([1,2,3,4][1])
 *   if any inputs have "0" -> button pressed may have  1 4 7 * (in col 0)
 * -If no inputs have "0" so the button is another cols so check the next column and so on
*/
uint8_t Get_Key(){
    K_pressed=0;
    for(cur_Col=0;cur_Col<num_Cols;cur_Col++)
    {
        GPIOPinWrite(Cols_port,Cols,0xFF);              //Set all outputs to 1
        GPIOPinWrite(Cols_port,ColPins[cur_Col],0);     //Repeatedly clear one column
        for(cur_Row=0;cur_Row<num_Rows;cur_Row++)             //Check to see if the rows have "0"
        {                                                     //Specify the button in that column
            if(GPIOPinRead(Rows_port, RowPins[cur_Row])==0) //if true return [row][col]
            K_pressed=Keypad[cur_Row][cur_Col];                   //else check another cols
        }
    }
    SysCtlDelay(SysCtlClockGet()/30);
    return K_pressed;
}

static void
KeypadTask(void *pvParameters)
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
        if (Check_KPad()==1){
            Test=Get_Key();

            //
            // Pass the value of the button pressed to LEDTask.
            //
            if (Test>0){
                if (xQueueSend(g_pKeypadQueue, &Test, 100 / portTICK_RATE_MS) == pdPASS)

                    //printf("\nidk\n");
                    temp=1;

                }else{
                    temp=0;
                }
        }
            //
            //            if (Test<10){
            //
            //
            //             }
            //             else
            //             {
            //                 idk=1;
            //                 idk=1;
            //                 idk=1;
            //                 idk=1;
            //                 idk=1;
            //             }
         //}


        vTaskDelayUntil(&ui32WakeTime, 100 / portTICK_RATE_MS);

    }
}

uint32_t
KeypadTaskInit(void)
{

    // Configuration of keyboard
    GPIO_Keypad_Init();



    //
    // Create the LED task.
    //
    if(xTaskCreate(KeypadTask, (const portCHAR *)"Keypad", KEYPADSTACKSIZE, NULL,
                   tskIDLE_PRIORITY + PRIORITY_KEYPAD_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //
    // Success.
    //
    return(0);
}



