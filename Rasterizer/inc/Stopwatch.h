#pragma once
#include <chrono>
#include <iostream>
#include <string>

class Stopwatch {

public:

	Stopwatch();
	void Restart();
	std::string ToString() const;
	double GetElapsed() const;

	//print the time elapsed in nanoseconds
	friend std::ostream& operator<<(std::ostream& os, const Stopwatch& s);

private:

	std::chrono::time_point<std::chrono::high_resolution_clock> start;
};