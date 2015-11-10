//*****************************************************************************
//
// hello.c - Simple hello world example.
//
// Copyright (c) 2006-2013 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 10007 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "stdio.h"
#include "drivers/rit128x96x4.h"
#include "utils/ustdlib.h"

#define X_MIN 0
#define X_MAX 120
#define Y_MIN 0
#define Y_MAX 88
#define X_WALL_SPACER 5

volatile unsigned long g_ulSystemClock;

volatile unsigned int g_millisecond_hundredths_counter;
volatile unsigned int g_millisecond_tenths_counter;
volatile unsigned int g_seconds_counter;
volatile unsigned int g_minutes_counter;

// docs say x is 96, y is 128
// x max is 120
// y max is 88
volatile unsigned int g_player_x_axis_counter = 0;
volatile unsigned int g_player_y_axis_counter = 44;

volatile unsigned int g_opponent_x_axis_counter = X_MAX-1;
volatile unsigned int g_opponent_y_axis_counter = 44;

volatile float g_ball_y_axis_counter = 44.0;
volatile unsigned int g_ball_x_axis_counter = 60;

volatile unsigned int g_ball_x_step = 0;
volatile float g_ball_y_step = 0;

// 0 == left, 1 == right
volatile unsigned int g_ball_x_direction = 0;
// 0 == up, 1 == down
volatile unsigned int g_ball_y_direction = 0;

// Game state
volatile unsigned int g_game_active = 1;

int IsYBounceable(int bounceboard_y, int ball_y) {

    int isBounceable = 0;

    if (bounceboard_y + 8 > ball_y
        && bounceboard_y - 8 < ball_y
    ) {
        isBounceable = 1;
        RIT128x96x4StringDraw("bounceable A", 44, 5, 11);
    }

    return isBounceable;
}

void CollisionDetector(void) {
    // Hit the player
    if (g_ball_x_axis_counter == g_player_x_axis_counter + X_WALL_SPACER
        &&
        IsYBounceable(g_player_y_axis_counter, g_ball_y_axis_counter) == 1
        &&
        g_ball_x_direction == 0
        //IsYBounceable(g_player_y_axis_counter, g_ball_y_axis_counter) == 1
    ) {
        g_ball_x_direction = 1;

        g_ball_y_step = 0.5;
        RIT128x96x4StringDraw("if A", 44, 0, 11);
    }
    // Hit the opponent
    else if (g_ball_x_axis_counter == g_opponent_x_axis_counter - X_WALL_SPACER
        && g_ball_y_axis_counter == g_opponent_y_axis_counter
        && g_ball_x_direction == 1
    ) {
        g_ball_x_direction = 0;
        RIT128x96x4StringDraw("if B", 44, 0, 11);
    }
    // Hit the wall
    else if (g_ball_x_axis_counter == X_MIN || g_ball_x_axis_counter == X_MAX) {
        g_game_active = 0;
    }
    // Hit the ceiling
    else if (g_ball_x_axis_counter != X_MIN
            && g_ball_x_axis_counter != X_MAX
            && g_ball_y_axis_counter == Y_MIN
    ) {
        g_ball_y_direction = 0;
    }
    // Hit the floor
    else if (g_ball_x_axis_counter != X_MIN
            && g_ball_x_axis_counter != X_MAX
            && g_ball_y_axis_counter == Y_MAX
    ) {
        g_ball_y_direction = 1;
    }
}

void BallMovement(void) {
    if (g_ball_x_direction == 0) {
        g_ball_x_axis_counter--;

    }
    else {
        g_ball_x_axis_counter++;

        g_ball_y_axis_counter += g_ball_y_step * 1;
    }

    if (g_ball_y_direction == 0) {
        g_ball_y_axis_counter += g_ball_y_step * 1;
    }
    else {
        g_ball_y_axis_counter -= g_ball_y_step * 1;
    }


    char output[10];

    usprintf(output, "%d,%d", g_ball_x_axis_counter, g_ball_y_axis_counter);

    RIT128x96x4StringDraw(output, 25, 10, 15);


}

// Interrupt handler to catch button presses and start the scrolling function
void BallMovementAnimation(void) {


    RIT128x96x4StringDraw(" ", g_ball_x_axis_counter - 1, g_ball_y_axis_counter - 1, 11);
    RIT128x96x4StringDraw(" ", g_ball_x_axis_counter - 2, g_ball_y_axis_counter - 2, 11);
    RIT128x96x4StringDraw(" ", g_ball_x_axis_counter + 2, g_ball_y_axis_counter + 2, 11);
    RIT128x96x4StringDraw(" ", g_ball_x_axis_counter + 1, g_ball_y_axis_counter + 1, 11);
    RIT128x96x4StringDraw("*", g_ball_x_axis_counter, g_ball_y_axis_counter, 11);

}

void PlayerMovementAnimation(void) {
    //const char time[15];
    //usprintf(time, "   %02d:%02d.%d   ", g_uiMinutes, g_uiSeconds, g_uiTenths);

    RIT128x96x4StringDraw(" ", g_player_x_axis_counter, g_player_y_axis_counter - 1, 11);
    RIT128x96x4StringDraw(" ", g_player_x_axis_counter, g_player_y_axis_counter - 2, 11);
    RIT128x96x4StringDraw(" ", g_player_x_axis_counter, g_player_y_axis_counter + 2, 11);
    RIT128x96x4StringDraw(" ", g_player_x_axis_counter, g_player_y_axis_counter + 1, 11);
    RIT128x96x4StringDraw(" ", g_player_x_axis_counter, g_player_y_axis_counter, 11);
    RIT128x96x4StringDraw("|", g_player_x_axis_counter, g_player_y_axis_counter, 11);

}

void OpponentMovementAnimation(void) {
    //const char time[15];
    //usprintf(time, "   %02d:%02d.%d   ", g_uiMinutes, g_uiSeconds, g_uiTenths);

    RIT128x96x4StringDraw("", g_opponent_x_axis_counter, g_opponent_y_axis_counter - 2, 11);
    RIT128x96x4StringDraw("|", g_opponent_x_axis_counter, g_opponent_y_axis_counter, 11);
    RIT128x96x4StringDraw("", g_opponent_x_axis_counter, g_opponent_y_axis_counter + 2, 11);

}


void
GPIOEIntHandler(void)
{
    GPIOPinIntClear(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    unsigned long ulData;

    //
    // Read the state of the push buttons.
    //
    ulData = (GPIOPinRead(GPIO_PORTE_BASE, (GPIO_PIN_0 | GPIO_PIN_1  | GPIO_PIN_2 | GPIO_PIN_3))) & 0x000f;
    ulData = ulData ^ 0x0f;
    // UP
    if (ulData == 1)
    {
        if (g_player_y_axis_counter > 0) {
            g_player_y_axis_counter--;

            char output[10];

            usprintf(output, "%d", g_player_y_axis_counter);

            RIT128x96x4StringDraw(output, 5, 10, 15);
        }
    }
    // DOWN
    if (ulData == 2)
    {
        if (g_player_y_axis_counter < Y_MAX-1) {
            g_player_y_axis_counter++;

            char output[10];

            usprintf(output, "%d", g_player_y_axis_counter);

            RIT128x96x4StringDraw(output, 5, 10, 15);
        }
    }
    if (ulData == 4)
    {

    }
    if (ulData == 8)
    {

    }

}





//*****************************************************************************
//
// Handles the SysTick timeout interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    if (g_game_active == 1) {

       CollisionDetector();

       BallMovement();

       PlayerMovementAnimation();
       OpponentMovementAnimation();
       BallMovementAnimation();
       /*

       g_millisecond_hundredths_counter++;

       if(g_millisecond_hundredths_counter > 99)
       {
           CollisionDetector();

           BallMovement();

           PlayerMovementAnimation();
           OpponentMovementAnimation();
           BallMovementAnimation();

           g_millisecond_hundredths_counter = 0;
           g_millisecond_tenths_counter++;
           if(g_millisecond_tenths_counter > 9)
           {
               g_millisecond_tenths_counter = 0;
               g_seconds_counter++;

               if(g_seconds_counter > 59)
               {
                   g_seconds_counter = 0;
                   g_minutes_counter++;
                   if(g_minutes_counter > 59)
                   {
                       g_minutes_counter = 0;
                   }
               }
           }
        }
        */
    }
}


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// Main program.
//
//*****************************************************************************
int
main(void)
{
    volatile int iDelay;

    //
    // Set the clocking to run directly from the crystal.
    //
    //
    // Set the clocking to run at 50MHz from the PLL.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);
    //
    // Get the system clock speed.
    //
    g_ulSystemClock = SysCtlClockGet();

    //
    // Enable the peripherals used by this example.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    //
    // Configure the push button as an input and enable the pin to interrupt on
    // the falling edge (i.e. when the push button is pressed).
    //
    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);
    GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_FALLING_EDGE);
    GPIOPinIntEnable(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    IntEnable(INT_GPIOE);

    //
    // Initialize the OLED display and write status.
    //
    RIT128x96x4Init(1000000);

    //
    // Set up and enable the SysTick timer.  It will be used as a reference
    // for delay loops in the interrupt handlers.  The SysTick timer period
    // will be set up for one second.
    //
    //SysTickPeriodSet(g_ulSystemClock / 1000 );
    SysTickPeriodSet(g_ulSystemClock / 50 );

    SysTickIntEnable();

    SysTickEnable();


    //
    // Enable interrupts to the processor.
    //
    IntMasterEnable();

    //
    // Enable the interrupts.
    //
    IntEnable(INT_GPIOE);

    while(1)
    {

    }
}

