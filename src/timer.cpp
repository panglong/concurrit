/**
 * Copyright (c) 2010-2011,
 * Tayfun Elmas    <elmas@cs.berkeley.edu>
 * All rights reserved.
 * <p/>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * <p/>
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * <p/>
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * <p/>
 * 3. The names of the contributors may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 * <p/>
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "concurrit.h"

namespace concurrit {

/********************************************************************************/

Timer::Timer(std::string name) {
	startTime.tv_sec = startTime.tv_usec = 0;
	endTime.tv_sec = endTime.tv_usec = 0;
	stopped = false;
	this->name = name;
}

/********************************************************************************/
Timer::~Timer() {
}

/********************************************************************************/

void Timer::gettimeofday_(timeval* t) {
	if (gettimeofday(t, NULL)) {
		printf("Failed to get the end time!");
		exit(-1);
	}
}

/********************************************************************************/

void Timer::start() {
	stopped = false; // reset stop flag
	gettimeofday_(&startTime);
	endTime = startTime;
}

/********************************************************************************/

void Timer::stop() {
	stopped = true; // set timer stopped flag
	gettimeofday_(&endTime);
	bool is_positive = (timeval_subtract_(&elapsedTime, &endTime, &startTime) == 0);
	safe_assert(is_positive);
}

/********************************************************************************/

double Timer::getElapsedTimeInMicroSec() {
	timeval diff = getElapsedTime();

	return (diff.tv_sec * 1000000.0) + diff.tv_usec;
}

/********************************************************************************/

double Timer::getElapsedTimeInMilliSec() {
	return this->getElapsedTimeInMicroSec() * 0.001;
}

/********************************************************************************/

double Timer::getElapsedTimeInSec() {
	return this->getElapsedTimeInMicroSec() * 0.000001;
}

/********************************************************************************/

double Timer::getElapsedTimeInMin() {
	return this->getElapsedTimeInSec() * (0.0166666667);
}

/********************************************************************************/

double Timer::getElapsedTimeInHours() {
	return this->getElapsedTimeInMin() * (0.0166666667);
}

/********************************************************************************/

double Timer::getElapsedTimeInDays() {
	return this->getElapsedTimeInHours() * (0.0416666667);
}

/********************************************************************************/

timeval Timer::getElapsedTime() {
	if (!stopped) {
		gettimeofday_(&endTime);
		bool is_positive = (timeval_subtract_(&elapsedTime, &endTime, &startTime) == 0);
		safe_assert(is_positive);
	}
	return elapsedTime;
}

/********************************************************************************/

std::string Timer::StartTimeToString() {
	return timeval_to_string_(&startTime);
}

std::string Timer::EndTimeToString() {
	return timeval_to_string_(&endTime);
}

/********************************************************************************/

std::string Timer::ElapsedTimeToString() {
	char buff[64];
	double h = getElapsedTimeInHours();
	double m = getElapsedTimeInMin() - (h * 60.0);
	double s = getElapsedTimeInSec() - (m * 60.0);
	double ml = getElapsedTimeInMilliSec() - (s * 1000.0);
	double mc = getElapsedTimeInMicroSec() - (ml * 1000.0);
	sprintf(buff, "%.2f h:%.2f m:%.2f s:%.2f ml:%.2f mc", h, m, s, ml, mc);
	return std::string(buff);
}

/********************************************************************************/

std::string Timer::ToString() {
	std::stringstream s;
	s << name << ":\n";
	s << "Search started: " << StartTimeToString() << "\n";
	s << "Search ended: " << EndTimeToString() << "\n";
	s << "Elapsed time: " << ElapsedTimeToString() << "\n";
	return s.str();
}

/********************************************************************************/

// this is from GNU C library manual
/* Subtract the `struct timeval' values X and Y,
 storing the result in RESULT.
 Return 1 if the difference is negative, otherwise 0.  */

int Timer::timeval_subtract_(struct timeval *result, struct timeval *_x,
		struct timeval *_y) {
	struct timeval x = *_x;
	struct timeval y = *_y;
	/* Perform the carry for the later subtraction by updating y. */
	if (x.tv_usec < y.tv_usec) {
		int nsec = (y.tv_usec - x.tv_usec) / 1000000 + 1;
		y.tv_usec -= 1000000 * nsec;
		y.tv_sec += nsec;
	}
	if (x.tv_usec - y.tv_usec > 1000000) {
		int nsec = (y.tv_usec - x.tv_usec) / 1000000;
		y.tv_usec += 1000000 * nsec;
		y.tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 tv_usec is certainly positive. */
	result->tv_sec = x.tv_sec - y.tv_sec;
	result->tv_usec = x.tv_usec - y.tv_usec;

	/* Return 1 if result is negative. */
	return x.tv_sec < y.tv_sec;
}

/********************************************************************************/

std::string Timer::timeval_to_string_(timeval* tv) {
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[64], buf[64];

	nowtime = tv->tv_sec;
	nowtm = localtime(&nowtime);
	strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);
	snprintf(buf, sizeof(buf), "%s.%06d", tmbuf, (int) tv->tv_usec);

	return std::string(buf);
}

/********************************************************************************/

} // end namespace
