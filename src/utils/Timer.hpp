#pragma once

#include <chrono>
#include <string>

class Timer {
public:
	Timer();

	void reset();


	typedef std::chrono::duration<double> Seconds;
	typedef std::chrono::duration<double, std::milli> Millis;

	template<typename T>
	T getDuration() const {
		return std::chrono::duration_cast<T>(std::chrono::high_resolution_clock::now() - m_last_sample);
	}

	void printSeconds(const std::string& id) const;

private:

	typedef std::chrono::high_resolution_clock::time_point TimePoint;

	TimePoint m_last_sample;
};