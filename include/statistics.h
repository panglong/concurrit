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
#include "timer.h"

namespace concurrit {

class Counter {
public:
	Counter(std::string name = "") : name_(name), value_(0) {}
	~Counter() {}

	void increment() {
		++value_;
	}
	void reset(std::string name = "") {
		if(name != "") {
			name_ = name;
		}
		value_ = 0;
	}
	std::string ToString() {
		std::stringstream s;
		s << name_ << ": " << value_;
		return s.str();
	}
	operator unsigned () {
		return value_;
	}
private:
	DECL_FIELD(std::string, name)
	DECL_FIELD(unsigned, value)
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

	}

	std::string ToString() {
		std::stringstream s;

		for(TimerMap::iterator itr = timers_.begin(); itr != timers_.end(); ++itr) {
			s << itr->second.ToString();
		}

		for(CounterMap::iterator itr = counters_.begin(); itr != counters_.end(); ++itr) {
			s << itr->second.ToString() << std::endl;
		}

		return s.str();
	}

	Timer& timer(std::string name) {
		TimerMap::iterator itr = timers_.find(name);
		if(itr == timers_.end()) {
			timers_[name] = Timer(name);
		}
		return timers_[name];
	}

	Counter& counter(std::string name) {
		CounterMap::iterator itr = counters_.find(name);
		if(itr == counters_.end()) {
			counters_[name] = Counter(name);
		}
		return counters_[name];
	}


private:
	DECL_FIELD_REF(TimerMap, timers)
	DECL_FIELD_REF(CounterMap, counters)
};

} // end namespace


#endif /* STATISTICS_H_ */
