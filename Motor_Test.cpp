/**
 * REXUS PIOneERS - Pi_1
 * Motor_Test.cpp
 * Purpose: Program for vacuum test of the motor.
 *
 * @author David Amison
 * @version 1.0 10/10/2017
 */

#include <unistd.h>
#include <fstream>
#include <wiringPi.h>
#include <stdint.h>
#include <iostream>  // For std::cout and std::cin

int MOTOR_CW = 4;
int MOTOR_ACW = 5;
int ENCODER_IN = 0;

int g_count = 0;

void encoder_pulse() {
	piLock(1);
	g_count++;
	piUnlock(1);
}

int main(int argc, char* argv[]) {
	if (argc != 1) {
		std::cout << "Command should be of the form: ./motor [filename]" << std::endl;
		return 1;
	}
	char* filename = argv[0];
	std::ofstream outf(filename);

	wiringPiSetup();
	pinMode(MOTOR_CW, OUTPUT);
	pinMode(MOTOR_ACW, OUTPUT);
	wiringPiISR(MOTOR_IN, INT_EDGE_RISING, encoder_pulse);

	uint32_t start = millis();
	uint32_t elapsed;
	digitalWrite(MOTOR_CW, 1);
	delay(100);
	while ((elapsed = millis() - start) < 30000) {
		piLock(1);
		int count = g_count;
		piUnlock(1);
		outf < "Count: " << count << "   Elapsed Time (ms): " << elapsed << std::endl;
		fprintf(stdout, "Count: %d   Elapsed Time (ms): %d", count, elapsed);
		delay(100);
	}
	return 0;
}

