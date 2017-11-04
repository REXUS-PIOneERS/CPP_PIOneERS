/**
 * REXUS PIOneERS - Pi_1
 * camera.h
 * Purpose: Function declarations for implementation of camera class for video
 *
 * @author David Amison
 * @version 1.1 10/10/2017
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <unistd.h>
#include <iostream>
#include <string>
#include "timing/timer.h"
#include <fstream>

class Logger {
private:
	std::string _filename;
	std::string _this_filename;
	std::ofstream _outf;
	Timer _tmr;

public:
	Logger(std::string filename);

	void start_log();

	void reopen_log();

	std::ostream& operator()(std::string str);

	void stop_log();

	~Logger() {
		stop_log();
	}
};

#endif /* LOGGER_H */
