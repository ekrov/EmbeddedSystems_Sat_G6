/*
 * buzzer_pwm.c
 *
 *  Created on: 22/12/2023
 *      Author: carlo
 */

#include "buzzer_pwm.h"

extern xSemaphoreHandle g_BuzzerSemaphore;


void configurePWM(void)
{
    // Enable the PWM0 peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    // Wait for the PWM0 module to be ready
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0))
    {
    }

    // Enable the PWM0 output pin (PWM0/PB6)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinConfigure(GPIO_PB6_M0PWM0);
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_6);

    // Configure the PWM generator for count down mode with immediate updates
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    // Set the period of the PWM signal (frequency)
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, SysCtlClockGet() / 440); // 440 Hz is the frequency of A4

    // Set the initial duty cycle to 50%
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, SysCtlClockGet() / (2 * 440));

    // Enable the PWM generator
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);

    // Enable the PWM output
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);

    // Start the PWM generator
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
}

void playSong(void){
    // Array representing the frequency of musical notes (in Hz)
        uint32_t noteFrequencies[] = {262, 294, 330, 349, 392, 440, 494, 523};
        volatile uint32_t i;

        for ( i = 0; i < (sizeof(noteFrequencies) / sizeof(noteFrequencies[0])); i++)
        {
            // Set the PWM frequency to play the musical note
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, SysCtlClockGet() / (2 * noteFrequencies[i]));

            // Delay to play the note for a certain duration
            SysCtlDelay(SysCtlClockGet() / 4); // Adjust the delay for the desired duration
        }

        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 0);

}


static void
BuzzerTask(void *pvParameters)
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
        if (xSemaphoreTake(g_BuzzerSemaphore, 0)== pdTRUE ){

            playSong();

            //configurePWM();
        }


        vTaskDelayUntil(&ui32WakeTime, 1000 / portTICK_RATE_MS);

    }
}

uint32_t
BuzzerTaskInit(void)
{

    configurePWM();
    //
    // Create the buzzer task.
    //
    if(xTaskCreate(BuzzerTask, (const portCHAR *)"Buzzer", BUZZERSTACKSIZE, NULL,
                   tskIDLE_PRIORITY + PRIORITY_BUZZER_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //
    // Success.
    //
    return(0);
}


