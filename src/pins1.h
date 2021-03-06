/**
 * REXUS PIOneERS - Pi_1
 * pins.h
 * Purpose: Defines all pin numbers used in the main raspi1 and raspi2 programs
 *
 * @author David Amison
 * @version 1.0 28/10/2017
 */

#ifndef PINS1_H
#define PINS1_H

// Signals for main logic control
#define LO    29
#define SOE    28
#define SODS   26
#define LAUNCH_MODE  11

// Pins for control of the motor
#define MOTOR_CW  24
#define MOTOR_ACW  25
#define MOTOR_IN  0

// Connection between pi 1 and 2
#define ALIVE   3

#endif /* PINS1_H */

