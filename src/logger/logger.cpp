/**
 * REXUS PIOneERS - Pi_1
 * camera.cpp
 * Purpose: Implementation of functions in the PiCamera class
 *
 * @author David Amison
 * @version 2.0 10/10/2017
 */

#include <string>
#include <sstream>
#include <stdint.h>
#include <fstream>
#include <iomanip>
#include "logger.h"
#include "timing/timer.h"

std::string time(Timer tmr) {
	std::stringstream ss;
	uint64_t time = tmr.elapsed();
	int hr = time / 3600000;
	time -= hr * 3600000;
	int min = time / 60000;
	time -= min * 60000;
	int sec = time / 1000;
	time -= sec * 1000;
	ss << std::setfill('0') << std::setw(2) << hr << ":" << min << ":" <<
			sec << ":" << time;
	return ss.str();
}

Logger::Logger(std::string filename) {
	_filename = filename;
}

void Logger::start_log() {
	_tmr.reset();
	std::ostringstream ss;
	ss << _filename << ".txt";
	_this_filename = ss.str();
	_outf.open(_this_filename, std::ofstream::app);
	_outf << "\n[New run of log file at " << Timer::str_datetime() << " ]\n" << std::endl;
}

void Logger::child_log() {
	std::stringstream ss;
	ss << _filename << "_child.txt";
	_this_filename = ss.str();
	_outf.close();
	_outf.clear();
	_outf.open(_this_filename, std::ofstream::app);
	_outf << "\n[New run of log file at " << Timer::str_datetime() << " ]\n" << std::endl;
}

std::ostream& Logger::operator()(std::string str) {
	return _outf << std::endl << str << "(" << time(_tmr) << "): ";
}

void Logger::stop_log() {
	_outf.close();
}
