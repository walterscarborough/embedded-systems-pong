# Embedded Systems Pong

###  Asim Saleem Phathekhan and Walter Scarborough

We have created a version of Pong for the LM3S8962 microcontroller board.

Our aim was to implement and flash a playable version of the game to our board and provide the default functionalities that are part of the original game.

This project is written in embedded C, and uses some vendor libraries that are specific to the LM3S8962 board. It would probably be pretty straightforward to port this to another system - changes would mostly need to be done to the system clock handler and the screen display functions.

See it in action here: https://www.youtube.com/watch?v=6g5fnYg0RCw

## Opponent AI

The opponent AI has two primary modes of execution: one will perfectly match the bounce board location with the ball at all times, and the other will move linearly in a single direction with a small chance of random variation. If the first mode is followed exclusively, then the opponent AI becomes impossible to defeat. We felt that this would make the game less enjoyable, and introduced the second mode as a way of balancing out difficulty. We used a simple lottery scheduling algorithm to determine whether the opponent AI will act in the first or second mode. Furthermore, the second mode includes a second lottery scheduling algorithm to determine whether the opponent AI will move linearly or add a random movement. Lottery votes for execution modes and randomness are calculated on every systick interval. We tested the game by playing it to determine which voting values provided the best player experience, and in the end decided on:

* 70% chance of the opponent AI playing in the first mode (i.e. perfectly)
* 30% chance of the opponent AI playing in the second mode (i.e. not perfectly)

Within the second mode, we decided on the following voting values:

* 95% chance of the opponent AI moving linearly in a single direction until encountering the floor/ceiling
* 5% chance of the opponent AI moving in a random variation

The first mode (aka the “perfect” AI) works by having the opponent bounce board track the y-axis value of the ball and attempt to match it after every ball movement event during systick events. Since the AI always knows where the ball is currently located and can move faster than the player can push buttons, it is almost always able to reach the ball in time. This was an interesting discovery because we did not allow the opponent bounce board to skip its y-axis counter faster than single digit increments.

The second mode (aka the “simple” AI) works by having the opponent AI move in a linear direction until collision occurs, or its direction is changed by a lottery vote in favor of a random movement variation.

By combining both modes together and tuning our lottery voting values, we feel that we have created an AI that provides the player with an enjoyable level of difficulty. It would be possible to create alternate difficulty levels by selecting different voting values for both lotteries.

## Contributions

Please feel free to contribute! Pull requests are welcome. 

This project is licensed under the GPL v3 license. For more information, please see the LICENSE.txt file included in this repository.
