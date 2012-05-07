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

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include "common.h"

namespace concurrit {

class Timer
{
public:
    Timer(std::string name = "");
    ~Timer();

    void   reset();
    void   start();
    void   stop();
    void   recordElapsedTime();
    timeval getElapsedTime();
    double getElapsedTimeInSec();
    double getElapsedTimeInMilliSec();
    double getElapsedTimeInMicroSec();
    double getElapsedTimeInMin();
    double getElapsedTimeInHours();
    double getElapsedTimeInDays();
    std::string StartTimeToString();
    std::string EndTimeToString();
    std::string ElapsedTimeToString();
    std::string ToString();

protected:
    static void gettimeofday_(timeval* t);
    static int timeval_subtract_(struct timeval *result, struct timeval *_x, struct timeval *_y);
    static std::string timeval_to_string_(timeval* tv);

private:
    bool    started;
    bool    stopped;
    timeval startTime;
    timeval endTime;
    timeval elapsedTime;
    std::string name;

    double elapsedTimeInHours;
    double elapsedTimeInMinutes;
    double elapsedTimeInSeconds;
    double elapsedTimeInMilliSec;
    double elapsedTimeInMicroSec;
};

/********************************************************************************/

class Counter {
public:
	Counter(std::string name = "") : name_(name), value_(0) {}
	virtual ~Counter() {}

	virtual void increment(unsigned long k = 1) {
		value_ += k;
	}

	void reset(std::string name = "") {
		if(name != "") {
			name_ = name;
		}
		value_ = 0;
	}

	virtual std::string ToString() {
		return format_string("%s: %lu", name_.c_str(), value_);
	}
	operator unsigned long () {
		return value_;
	}
private:
	DECL_FIELD(std::string, name)
	DECL_FIELD(unsigned long, value)
};

/********************************************************************************/

class AvgCounter : public Counter {
public:
	AvgCounter(std::string name = "") : Counter(name), count_(0) {}
	~AvgCounter() {}

	// override
	void increment(unsigned long k = 1) {
		Counter::increment(k);
		++count_;
	}
	void reset(std::string name = "") {
		Counter::reset(name);
		count_ = 0;
	}

	// override
	std::string ToString() {
		return format_string("%s: Num: %lu | Total: %lu MicroSecs | Avg: %lu MicroSecs", name_.c_str(), count_, value_, (value_/count_));
	}
	operator unsigned long () {
		return (value_/count_);
	}
private:
	DECL_FIELD(unsigned long, count)
};

/********************************************************************************/

class Statistics {
	typedef std::map<std::string, Timer> TimerMap;
	typedef std::map<std::string, Counter> CounterMap;
public:
	Statistics() {
		Reset();
	}
	~Statistics() {}

	void Reset() {
		timers_.clear();
		counters_.clear();
	}

	std::string ToString() {
		std::stringstream s;

		s << "************* Statistics *************\n";

		for(TimerMap::iterator itr = timers_.begin(); itr != timers_.end(); ++itr) {
			s << itr->second.ToString() << std::endl;
		}

		for(CounterMap::iterator itr = counters_.begin(); itr != counters_.end(); ++itr) {
			s << itr->second.ToString() << std::endl;
		}

		return s.str();
	}

	Timer& timer(const std::string& name) {
		TimerMap::iterator itr = timers_.find(name);
		if(itr == timers_.end()) {
			timers_[name] = Timer(name);
		}
		return timers_[name];
	}

	Counter& counter(const std::string& name) {
		CounterMap::iterator itr = counters_.find(name);
		if(itr == counters_.end()) {
			counters_[name] = Counter(name);
		}
		return counters_[name];
	}

	AvgCounter& avg_counter(const std::string& name) {
		CounterMap::iterator itr = counters_.find(name);
		if(itr == counters_.end()) {
			counters_[name] = AvgCounter(name);
		}
		return static_cast<AvgCounter&>(counters_[name]);
	}



private:
	DECL_FIELD_REF(TimerMap, timers)
	DECL_FIELD_REF(CounterMap, counters)
};

} // end namespace


#endif /* STATISTICS_H_ */
