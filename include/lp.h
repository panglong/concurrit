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


#ifndef LP_H_
#define LP_H_

#include "common.h"
#include "vc.h"
#include "schedule.h"

namespace concurrit {

/********************************************************************************/

struct point_t {
public:
	THREADID		tid_;
	std::string 	label_;
	unsigned int 	count_;

	point_t(THREADID tid, std::string label, unsigned int count)
	: tid_(tid), label_(label), count_(count) {
		safe_assert(count_ >= 1);
	}

	bool operator==(const point_t& p) const {
		return tid_ == p.tid_ && label_ == p.label_ && count_ == p.count_;
	}

	bool operator!=(const point_t& p) const {
		return !(operator==(p));
	}

	bool operator<(const point_t& p) const {
		if(label_ != p.label_) return label_ < p.label_;
		if(count_ != p.count_) return count_ < p.count_;
		//if(tid_ != p.tid_) return tid_ < p.tid_;
		return false;
	}

	std::string ToString() const {
		std::string s = label_;
		if(count_ > 1) s += ",";
		return s;
	}
};

/********************************************************************************/

class Coverage;

struct tuple_t {
public:
	tuple_t(int len, point_t args[]) {
		for(int i = 0; i < len; ++i) {
			points_.push_back(args[i]);
		}
	}

	int len() const {
		return points_.size();
	}

	bool operator==(const tuple_t& t) const {
		if(t.len() != len()) {
			return false;
		}

		for(int i = 0; i < len(); ++i) {
			if(points_[i] != t.points_[i]) {
				return false;
			}
		}

		return true;
	}

	bool operator!=(const tuple_t& t) const {
		return !(operator==(t));
	}

	bool operator<(const tuple_t& t) const {
		if(t.len() != len()) {
			return len() < t.len();
		}

		for(int i = 0; i < len(); ++i) {
			if(points_[i] != t.points_[i]) {
				return points_[i] < t.points_[i];
			}
		}

		return false;
	}

	std::string ToString() const {
		std::stringstream s;
		s << "(";
		const char* comma = "";
		for(int i = 0; i < len(); ++i) {
			s << comma << points_[i].ToString();
			comma = ", ";
		}
		s << ")";
		return s.str();
	}

	Coverage* Derive();

private:
	DECL_FIELD(std::vector<point_t>, points)
};

/********************************************************************************/

struct tuple_compare {
	bool operator()(const tuple_t& t1, const tuple_t& t2) const {
		return t1 < t2;
	}
};

typedef std::set<tuple_t, tuple_compare> TupleSet;

/********************************************************************************/

class Coverage {
public:
	Coverage() {}

	~Coverage() {}

	bool AddTuple(tuple_t& tuple) {
		if(tuples_.find(tuple) != tuples_.end()) {
			return false;
		}
		tuples_.insert(tuple);
		return true;
	}

	std::string ToString() const {
		std::stringstream s;
		if(tuples_.empty()) {
			s << "No coverage!\n";
		} else {
			s << "Tuples:\n";
			for(TupleSet::iterator itr = tuples_.begin(); itr != tuples_.end(); ++itr) {
				s << (*itr).ToString() << "\n";
			}
		}
		return s.str();
	}

	// returns this
	bool AddAll(Coverage* c) {
		bool updated = false;
		for(TupleSet::iterator itr = c->tuples_.begin(); itr != c->tuples_.end(); ++itr) {
			tuple_t tuple = (*itr);
			if(AddTuple(tuple)) {
				updated = true;
			}
		}
		return updated;
	}

	void Clear() {
		tuples_.clear();
	}

	Coverage* Clone() {
		Coverage* cov = new Coverage();
		*cov = *this;
		return cov;
	}

	Coverage* Derive();

	TupleSet* GetTuples() {
		return &tuples_;
	}

private:
	DECL_FIELD_REF(TupleSet, tuples)

};

/********************************************************************************/

class VCTracker {
public:
	VCTracker() {
		Restart();
	}

	~VCTracker(){
		for(std::map<ADDRINT, std::set<SchedulePoint*>*>::iterator itr = memToLastAccess_.begin();
				itr != memToLastAccess_.end(); ++itr) {
			std::set<SchedulePoint*>* accesses = itr->second;
			safe_assert(accesses != NULL);
			accesses->clear();
			delete accesses;
		}
		memToLastAccess_.clear();
		memToVC_.clear();
	}

	void Restart() {
		currentTime_ = 1;
		memToVC_.clear();
		for(std::map<ADDRINT, std::set<SchedulePoint*>*>::iterator itr = memToLastAccess_.begin();
				itr != memToLastAccess_.end(); ++itr) {
			std::set<SchedulePoint*>* accesses = itr->second;
			safe_assert(accesses != NULL);
			accesses->clear();
		}
	}

	void OnAccess(SchedulePoint* point, Coverage* coverage);
	void UpdateBacktrackSets(SchedulePoint* point);

protected:
	std::set<SchedulePoint*>* GetLastAccesses(ADDRINT mem, bool createIfAbsent = false);
	void HandleImmediateRace(SchedulePoint* point, SchedulePoint* currentPoint, Coverage* coverage);


private:
	DECL_FIELD(vctime_t, currentTime)
	std::map<ADDRINT, VC> memToVC_;
	std::map<ADDRINT, std::set<SchedulePoint*>*> memToLastAccess_;
};

/********************************************************************************/

}

#endif /* LP_H_ */
