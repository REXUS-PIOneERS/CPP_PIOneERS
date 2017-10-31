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

namespace log {

	class Logger {
	public:
		Logger(std::string filename);

		void start_log();

		template<typename T>
		friend std::ostream& operator<<(Logger &l, T output);

		void stop_log();

		~Logger() {
			stop_log();
		}

	private:
		std::string _filename;
		std::ofstream _outf;
		Timer _tmr;
	};
}

#endif /* LOGGER_H */
