//*****************************************************************************
//
// Asim Saleem Phathekhan <asim.saleem@utexas.edu>
// Walter Scarborough <walter.scarborough@utexas.edu>
//
// Pong - Using LM3S8962
// Embedded Systems Final Project
// 11/Nov/2015
//
// This is our implementation of pong on the LM3S8962 board!
//
// It features:
// * a balanced opponent AI
// * smooth animation
// * player and opponent score
// * realistic ball and bounce board collisions/physics
//
// We have added comments throughout the code to explain what each function does.
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

////////////////////////
// Pong Global Constants
////////////////////////

// Grid
#define X_MIN 0
#define X_MAX 120
#define Y_MIN 0
#define Y_MAX 88
#define X_WALL_SPACER 5

// Bounce boards
#define BOARD_TOLERANCE 7
#define BOARD_SHALLOW_ANGLE_OFFSET 3

// Ball
#define BALL_DIRECTION_UP 0
#define BALL_DIRECTION_DOWN 1
#define BALL_DIRECTION_LEFT 0
#define BALL_DIRECTION_RIGHT 1

#define BALL_X_ORIGIN 60
#define BALL_Y_ORIGIN 44.0

// AI
#define OPPONENT_DIRECTION_UP 0
#define OPPONENT_DIRECTION_DOWN 1

/////////////////
// Pong Variables
/////////////////

// Systick
volatile unsigned long g_ulSystemClock;

// Player Coordinates
volatile unsigned int g_player_x_axis_counter = X_MIN;
volatile unsigned int g_player_y_axis_counter = Y_MAX / 2;

// Opponent Coordinates
volatile unsigned int g_opponent_x_axis_counter = X_MAX-1;
volatile unsigned int g_opponent_y_axis_counter = Y_MAX / 2;
volatile unsigned int g_opponent_y_direction = OPPONENT_DIRECTION_UP;

// Ball Coordinates
volatile float g_ball_y_axis_counter = BALL_Y_ORIGIN;
volatile unsigned int g_ball_x_axis_counter = BALL_X_ORIGIN;

// Ball Angle
volatile unsigned int g_ball_x_step = 0;
volatile float g_ball_y_step = 0;

// Ball Movement
volatile unsigned int g_ball_x_direction = BALL_DIRECTION_LEFT;
volatile unsigned int g_ball_y_direction = BALL_DIRECTION_UP;

// General Game State
volatile unsigned int g_game_active = 1;
volatile unsigned int g_game_sleep = 0;
volatile unsigned int g_game_sleep_counter = 0;
volatile unsigned int g_player_score = 0;
volatile unsigned int g_opponent_score = 0;

//////////////////////
// Pong Game Functions
//////////////////////

// Determines which direction the ball should move after colliding with a bounce board
int BallDirectionForBounceboardCollision(int bounceboard_y, int ball_y) {
    int newBallDirection = 0;

    // bottom
    if (bounceboard_y + BOARD_TOLERANCE > ball_y
        && bounceboard_y < ball_y
    ) {
        newBallDirection = BALL_DIRECTION_DOWN;
    }
    // middle
    else if (bounceboard_y == ball_y) {
        newBallDirection = -1;
    }
    // top
    else if (bounceboard_y - BOARD_TOLERANCE < ball_y
        && bounceboard_y > ball_y
    ) {
        newBallDirection = BALL_DIRECTION_UP;
    }

    return newBallDirection;
}

// Determines the angle that the ball should move at after colliding with a bounce board
// We have set up 5 regions: bottom, middle-bottom, middle, middle-top, and top.
// The bottom and top regions will push the ball in the the widest outgoing angles.
float BallYBounceAngle(int bounceboard_y, int ball_y) {

    float ball_bounce_y_angle = 0.0;

    // bottom
    if (bounceboard_y + BOARD_TOLERANCE > ball_y
        && bounceboard_y < ball_y
    ) {
        ball_bounce_y_angle = 0.75;

    }
    // middle-bottom
    else if (bounceboard_y + BOARD_TOLERANCE - BOARD_SHALLOW_ANGLE_OFFSET > ball_y
            && bounceboard_y < ball_y
    ) {
        ball_bounce_y_angle = 0.2;
    }
    // middle
    else if (bounceboard_y == ball_y) {
        ball_bounce_y_angle = 0;
    }
    // middle-top
    else if (bounceboard_y - BOARD_TOLERANCE + BOARD_SHALLOW_ANGLE_OFFSET < ball_y
            && bounceboard_y > ball_y) {
        ball_bounce_y_angle = 0.2;
    }
    // top
    else if (bounceboard_y - BOARD_TOLERANCE < ball_y
        && bounceboard_y > ball_y
    ) {
        ball_bounce_y_angle = 0.75;
    }

    return ball_bounce_y_angle;
}

// Determines whether the ball y-axis position is within an tolerance range of the bounce board y-axis position
int IsYBounceable(int bounceboard_y, int ball_y) {

    int isBounceable = 0;

    if (bounceboard_y + BOARD_TOLERANCE > ball_y
        && bounceboard_y - BOARD_TOLERANCE < ball_y
    ) {
        isBounceable = 1;
    }

    return isBounceable;
}

// Determines whether the ball hits the player, opponent, or the wall
void CollisionDetector(void) {

    // Hit the player
    if (g_ball_x_axis_counter == (g_player_x_axis_counter + X_WALL_SPACER)
        &&
        IsYBounceable(g_player_y_axis_counter, g_ball_y_axis_counter) == 1
        &&
        g_ball_x_direction == BALL_DIRECTION_LEFT
    ) {

        g_ball_x_direction = BALL_DIRECTION_RIGHT;

        g_ball_y_step = BallYBounceAngle(g_player_y_axis_counter, g_ball_y_axis_counter);

        float newBallDirection = BallDirectionForBounceboardCollision(g_player_y_axis_counter, g_ball_y_axis_counter);

        if (newBallDirection == BALL_DIRECTION_DOWN) {
            g_ball_y_direction = BALL_DIRECTION_DOWN;
        }
        else if (newBallDirection == BALL_DIRECTION_UP) {
            g_ball_y_direction = BALL_DIRECTION_UP;
        }
    }
    // Hit the opponent
    else if (g_ball_x_axis_counter == g_opponent_x_axis_counter - X_WALL_SPACER
            &&
            IsYBounceable(g_opponent_y_axis_counter, g_ball_y_axis_counter) == 1
            &&
            g_ball_x_direction == BALL_DIRECTION_RIGHT
    ) {

        g_ball_x_direction = BALL_DIRECTION_LEFT;
        g_ball_y_step = BallYBounceAngle(g_opponent_y_axis_counter, g_ball_y_axis_counter);

        float newBallDirection = BallDirectionForBounceboardCollision(g_opponent_y_axis_counter, g_ball_y_axis_counter);

        if (newBallDirection == BALL_DIRECTION_DOWN) {
            g_ball_y_direction = BALL_DIRECTION_DOWN;
        }
        else if (newBallDirection == BALL_DIRECTION_UP) {
            g_ball_y_direction = BALL_DIRECTION_UP;
        }

    }
    // Hit the player wall
    else if (g_ball_x_axis_counter == X_MIN) {

        g_ball_y_step = 0;

        if (g_opponent_score == 9) {
            g_opponent_score++;

            g_game_active = 0;

            RIT128x96x4StringDraw("The CPU wins!", X_MAX / 5, Y_MAX / 2, 11);
        }
        else {
            g_opponent_score++;
            g_game_active = 0;

            RIT128x96x4StringDraw(" ", g_ball_x_axis_counter, g_ball_y_axis_counter, 11);

            g_game_sleep = 1;

            g_ball_y_axis_counter = BALL_Y_ORIGIN;
            g_ball_x_axis_counter = BALL_X_ORIGIN;

            g_ball_x_direction == BALL_DIRECTION_RIGHT;
        }
    }
    // Hit the opponent wall
    else if (g_ball_x_axis_counter == X_MAX) {

        g_ball_y_step = 0;

        if (g_player_score == 9) {
            g_player_score++;

            g_game_active = 0;

            RIT128x96x4StringDraw("You win!", X_MAX / 5, Y_MAX / 2, 11);
        }
        else {
            g_player_score++;

            g_game_active = 0;

            RIT128x96x4StringDraw(" ", g_ball_x_axis_counter, g_ball_y_axis_counter, 11);

            g_game_sleep = 1;

            g_ball_y_axis_counter = BALL_Y_ORIGIN;
            g_ball_x_axis_counter = BALL_X_ORIGIN;

            g_ball_x_direction == BALL_DIRECTION_LEFT;
        }
    }

}

// Moves the ball by incrementing its x-axis and y-axis counters. All y-axis movement will take the current angle into consideration.
void BallMovement(void) {
    if (g_ball_x_direction == BALL_DIRECTION_LEFT) {
        g_ball_x_axis_counter--;
    }
    else {
        g_ball_x_axis_counter++;
    }

    if (g_ball_y_direction == BALL_DIRECTION_DOWN) {

        if (g_ball_y_axis_counter < Y_MAX-1) {
            g_ball_y_axis_counter += g_ball_y_step;
        }
        else {
            g_ball_y_direction = BALL_DIRECTION_UP;
        }
    }
    else {

        if (g_ball_y_axis_counter > Y_MIN+1) {
            g_ball_y_axis_counter -= g_ball_y_step;
        }
        else {
            g_ball_y_direction = BALL_DIRECTION_DOWN;
        }
    }
}

// Moves the opponent board by invoking either an "invincible" or "linear" playing strategy.
// The strategy used is determined by a lottery scheduling algorithm.
// There's a higher chance of the AI using the "invincible" strategy, but by occasionally voting for the "linear" strategy it will eventually make a mistake
// and allow the player to score a point.
//
// The "linear" strategy has a second lottery vote to determine whether the board will truly move linearly,
// or incorporate a random variation in its movement.
void OpponentMovement(void) {

    // Take the vote for "invincible" or "linear" mode
    int invincibleVote = rand() % 100;

    // Voted for invincible mode
    if (invincibleVote < 70) {

        // Adjust the opponent movement to move its bounce board hit range to match the current ball location
        if (g_ball_y_axis_counter > g_opponent_y_axis_counter - 4
        ) {
            g_opponent_y_direction = OPPONENT_DIRECTION_DOWN;
        }
        else if (g_ball_y_axis_counter < g_opponent_y_axis_counter + 4
        ) {
            g_opponent_y_direction = OPPONENT_DIRECTION_UP;
        }
    }
    // Voted for linear mode
    else {

        // Take the vote for pure linear movement, or whether to incorporate a random variation
        int normalMovementVote = rand() % 100;

        // Voted for pure linear movement
        if (normalMovementVote > 95) {
            if (g_opponent_y_direction == OPPONENT_DIRECTION_UP) {
                g_opponent_y_direction = OPPONENT_DIRECTION_DOWN;
            }
            else {
                g_opponent_y_direction = OPPONENT_DIRECTION_UP;
            }
        }
    }

    // Update opponent y-axis counters for movement
    if (g_opponent_y_direction == OPPONENT_DIRECTION_UP) {
        if (g_opponent_y_axis_counter > Y_MIN) {
            g_opponent_y_axis_counter--;
        }
        else {
            g_opponent_y_direction = OPPONENT_DIRECTION_DOWN;
        }
    }
    else {
        if (g_opponent_y_axis_counter < Y_MAX) {
            g_opponent_y_axis_counter++;
        }
        else {
            g_opponent_y_direction = OPPONENT_DIRECTION_UP;
        }
    }
}

// Animate ball movement by drawing the ball's current position.
// Also cleans up space around the ball to prevent unwanted animation pixel "trails".
// Cleanup is done by drawing blank spaces around the ball.
void BallMovementAnimation(void) {

    int i = 1;
    for (i = 1; i <= 2; i++) {
        RIT128x96x4StringDraw(" ", g_ball_x_axis_counter - i, g_ball_y_axis_counter - i, 11);
        RIT128x96x4StringDraw(" ", g_ball_x_axis_counter + i, g_ball_y_axis_counter + i, 11);
    }

    RIT128x96x4StringDraw("*", g_ball_x_axis_counter, g_ball_y_axis_counter, 11);
}

// Animate player movement by drawing the player's current position.
// Also cleans up space around the player to prevent unwanted animation pixel "trails".
// Cleanup is done by drawing blank spaces around the player bounce board.
// The player has a fairly wide movement range to help the controls stay responsive,
// so there is quite a bit of cleanup that needs to be done.
void PlayerMovementAnimation(void) {

    int i = 1;
    for (i = 1; i <= 6; i++) {
        RIT128x96x4StringDraw(" ", g_player_x_axis_counter, g_player_y_axis_counter - i, 11);
        RIT128x96x4StringDraw(" ", g_player_x_axis_counter, g_player_y_axis_counter + i, 11);

    }

    RIT128x96x4StringDraw(" ", g_player_x_axis_counter, g_player_y_axis_counter, 11);
    RIT128x96x4StringDraw("|", g_player_x_axis_counter, g_player_y_axis_counter, 11);
}

// Animate opponent movement by drawing the opponent's current position.
// Also cleans up space around the opponent to prevent unwanted animation pixel "trails".
// Cleanup is done by drawing blank spaces around the opponent bounce board.
void OpponentMovementAnimation(void) {

    int i = 1;
    for (i = 1; i <= 2; i++) {
        RIT128x96x4StringDraw(" ", g_opponent_x_axis_counter, g_opponent_y_axis_counter - i, 11);
        RIT128x96x4StringDraw(" ", g_opponent_x_axis_counter, g_opponent_y_axis_counter + i, 11);
    }

    RIT128x96x4StringDraw(" ", g_opponent_x_axis_counter, g_opponent_y_axis_counter, 11);
    RIT128x96x4StringDraw("|", g_opponent_x_axis_counter, g_opponent_y_axis_counter, 11);
}

// Displays current score values on the screen.
// The player score appears in the top left hand corner of the screen.
// The opponent score appears in the top right hand corner of the screen.
void DisplayScores(void) {

    char playerScoreString[10];
    usprintf(playerScoreString, "%d", g_player_score);
    RIT128x96x4StringDraw(playerScoreString, X_MIN + 10, 0, 15);

    char opponentScoreString[10];
    usprintf(opponentScoreString, "%d", g_opponent_score);
    RIT128x96x4StringDraw(opponentScoreString, X_MAX - 10, 0, 15);
}

//////////////////////
// Pong Game Interrupts
//////////////////////

// Collect player button presses.
// Since the player bounce board can only move vertically along the y-axis, we are only interested
// in collecting input for the "up" and "down" buttons.
//
// Player animation is updated here to account for timing differences. Animation will not be smooth
// if we have to wait for the next systick event.
void GPIOEIntHandler(void) {
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
            g_player_y_axis_counter = g_player_y_axis_counter - 4;

            PlayerMovementAnimation();
        }
    }
    // DOWN
    if (ulData == 2)
    {
        if (g_player_y_axis_counter < Y_MAX-1) {
            g_player_y_axis_counter = g_player_y_axis_counter + 4;

            PlayerMovementAnimation();
        }
    }
}

// The entire game runs on systick intervals.
// It is important to detect incoming collisions before allowing any other automated (non player) movement.
// This helps keep the game play realistic (e.g. the ball can't fly through a bounce board, etc).
void SysTickIntHandler(void) {

    // Handle gameplay if the game is active
    if (g_game_active == 1) {

       CollisionDetector();

       BallMovement();

       OpponentMovement();

       PlayerMovementAnimation();
       OpponentMovementAnimation();
       BallMovementAnimation();

       DisplayScores();
    }
    // Count down to the next match if the game is paused due to a point being won.
    else if (g_game_sleep == 1) {
        g_game_sleep_counter++;

        if (g_game_sleep_counter < 30) {
            RIT128x96x4StringDraw("3", BALL_X_ORIGIN, BALL_Y_ORIGIN, 11);
        }
        else if (
            g_game_sleep_counter > 30
            && g_game_sleep_counter < 60
        ) {
            RIT128x96x4StringDraw("2", BALL_X_ORIGIN, BALL_Y_ORIGIN, 11);
        }
        else {
            RIT128x96x4StringDraw("1", BALL_X_ORIGIN, BALL_Y_ORIGIN, 11);
        }


        if (g_game_sleep_counter > 100) {
            RIT128x96x4StringDraw("*", BALL_X_ORIGIN, BALL_Y_ORIGIN, 11);

            g_game_sleep_counter = 0;
            g_game_sleep = 0;
            g_game_active = 1;
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
int main(void) {
    volatile int iDelay;

    //
    // Set the clocking to run directly from the crystal.
    //
    //
    // Set the clocking to /run at 50MHz from the PLL.
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

