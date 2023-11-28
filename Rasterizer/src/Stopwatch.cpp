#include "Stopwatch.h"

using namespace std;
using namespace std::chrono;

Stopwatch::Stopwatch(){
	start = high_resolution_clock::now();
}

void Stopwatch::Restart(){
	start = high_resolution_clock::now();
}

string Stopwatch::ToString() const{

	double elapsed_nano = GetElapsed();

	string ret{"Time elapsed: "};
	ret += to_string(elapsed_nano);
	ret += "ns";
	return ret;
}

double Stopwatch::GetElapsed() const{
	time_point<high_resolution_clock> elapsed = high_resolution_clock::now();
	nanoseconds elapsed_nano = elapsed - start;
	return elapsed_nano.count();
}

ostream& operator<<(ostream& os, const Stopwatch& s){
	os << s.ToString();
	return os;
}