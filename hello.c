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

volatile unsigned long g_ulSystemClock;

volatile unsigned int g_millisecond_hundredths_counter;
volatile unsigned int g_millisecond_tenths_counter;
volatile unsigned int g_seconds_counter;
volatile unsigned int g_minutes_counter;

// Max is ?
volatile unsigned int g_player_y_axis_counter = 12;
volatile unsigned int g_opponent_y_axis_counter = 12;


// Interrupt handler to catch button presses and start the scrolling function


void PlayerMovement(void) {
    //const char time[15];
    //usprintf(time, "   %02d:%02d.%d   ", g_uiMinutes, g_uiSeconds, g_uiTenths);

    RIT128x96x4StringDraw("", 0, g_player_y_axis_counter - 1, 11);
    RIT128x96x4StringDraw("", 0, g_player_y_axis_counter - 2, 11);
    RIT128x96x4StringDraw("", 0, g_player_y_axis_counter + 2, 11);
    RIT128x96x4StringDraw("", 0, g_player_y_axis_counter + 1, 11);
    RIT128x96x4StringDraw("", 0, g_player_y_axis_counter, 11);
    RIT128x96x4StringDraw("|", 0, g_player_y_axis_counter, 11);

}

void OpponentMovement(void) {
    //const char time[15];
    //usprintf(time, "   %02d:%02d.%d   ", g_uiMinutes, g_uiSeconds, g_uiTenths);

    RIT128x96x4StringDraw("", 80, g_opponent_y_axis_counter - 2, 11);
    RIT128x96x4StringDraw("|", 80, g_opponent_y_axis_counter, 11);
    RIT128x96x4StringDraw("", 80, g_opponent_y_axis_counter + 2, 11);

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
    	}
    }
    // DOWN
    if (ulData == 2)
    {
    	if (g_player_y_axis_counter < 50) {
    		g_player_y_axis_counter++;
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
	   g_millisecond_hundredths_counter++;

	   if(g_millisecond_hundredths_counter > 99)
	   {

		   PlayerMovement();
		   OpponentMovement();

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
    SysTickPeriodSet(g_ulSystemClock / 1000 );
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
