#include <wiringPi.h>
#include <unistd.h>
#include <iostream>
#include <signal.h>

const int MOTOR_CW = 24;
const int MOTOR_ACW = 25;

void signal_handler(int sig) {
	std::cout << "Stopping Motor" << std::endl;
	digitalWrite(MOTOR_CW, 0);
	digitalWrite(MOTOR_ACW, 0);
	exit(sig);
}

void count_encoder() {
	piLock(1);
	encoder_count++;
	piUnlock(1);
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage is boom_test [max_count]" << std::endl;
		return -1;
	}
	signal(SIGINT, signal_handler);
	wiringPiSetup();

	pinMode(MOTOR_CW, OUTPUT);
	pinMode(MOTOR_ACW, OUTPUT);

	digitalWrite(MOTOR_CW, 1);
	digitalWrite(MOTOR_ACW, 0);
	int max = atoi(argv[0]);
	int count = 0;
	while (count < max) {
		piLock(1);
		count = encoder_count;
		piUnlock(1);
		std::cout << "COUNT: " << count << std::endl;
		std::cout << "DIST: " << 0.126 * ((double) count / 600) << " m" << std::endl;
	}
}
