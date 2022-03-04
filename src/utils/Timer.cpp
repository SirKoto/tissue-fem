#include "Timer.hpp"

#include <iostream>

Timer::Timer()
{
	this->reset();
}

void Timer::reset()
{
	m_last_sample = std::chrono::high_resolution_clock::now();
}

void Timer::printSeconds(const std::string& id) const
{
	auto duration = this->getDuration<Seconds>();
	std::cout << "Elapsed " << duration.count() << " s. on " << id << std::endl;
}
