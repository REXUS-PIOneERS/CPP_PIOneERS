/**
 * REXUS PIOneERS - Pi_1
 * pins.h
 * Purpose: Defines all pin numbers used in the main raspi1 and raspi2 programs
 *
 * @author David Amison
 * @version 1.0 28/10/2017
 */

#ifndef PINS_H
#define PINS_H

// Signals for main logic control
#define LO    29
#define SOE    23
#define SODS   27
#define LAUNCH_MODE  11

// Pins for control of the motor
#define MOTOR_CW  24
#define MOTOR_ACW  25
#define MOTOR_IN  0

// Connection between pi 1 and 2
#define ALIVE   3

// Pins for control of burn wire
#define BURNWIRE  4

#endif /* PINS_H */

