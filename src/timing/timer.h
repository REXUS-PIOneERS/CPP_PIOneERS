#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <ctime>
#include <thread>
#include <stdint.h>

#include <sstream>
#include <string>
#include <iomanip>

class Timer {
public:

	Timer() : beg_(clock_::now()) {
	}

	void reset() {
		beg_ = clock_::now();
	}

	int32_t time() const {
		return (int32_t) std::chrono::duration_cast<millisecs_>(clock_::now().time_since_epoch()).count();
	}

	/**
	 * Return date-time in the format YY-MM-DDThhmmss
	 * @return string representing date and time
	 */
	static std::string str_datetime() {
		std::time_t tt = std::chrono::system_clock::to_time_t(clock_::now());
		std::tm tm = {0};
		gmtime_r(&tt, &tm);
		std::stringstream ss;
		ss << std::setfill('0') << std::setw(2) << tm.tm_year - 100 << "-" << 1+tm.tm_mon << "-" << tm.tm_mday << "T" <<
				tm.tm_hour << "-" << tm.tm_min << "-" << tm.tm_sec;
		return ss.str();
	}

	int32_t elapsed() const {
		return (int32_t) std::chrono::duration_cast<millisecs_>(clock_::now() - beg_).count();
	}

	static void sleep_ms(int ms) {
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}

private:
	typedef std::chrono::system_clock clock_;
	typedef std::chrono::duration<uint32_t, std::milli> millisecs_;
	std::chrono::time_point<clock_> beg_;
};

#endif /* TIMER_H */
