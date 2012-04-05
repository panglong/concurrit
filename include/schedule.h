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

#ifndef SCHEDULE_H_
#define SCHEDULE_H_

#include "common.h"
#include "coroutine.h"
#include "lp.h"
#include "serialize.h"

namespace concurrit {


/********************************************************************************/

// base class
class YieldPoint;
class TransferPoint;
class ChoicePoint;

class SchedulePoint : public Serializable {
public:
	SchedulePoint(): is_resolved_(false) {}
	virtual ~SchedulePoint() {}
	virtual YieldPoint* AsYield() { return NULL; }
	virtual TransferPoint* AsTransfer() { return NULL; }
	virtual ChoicePoint* AsChoice() { return NULL; }
	virtual bool IsTransfer() { return AsTransfer() != NULL; }
	virtual bool IsYield() { return AsYield() != NULL; }
	virtual bool IsChoice() { return AsChoice() != NULL; }

	virtual bool IsResolved() { return is_resolved_; }

	virtual std::string ToString() = 0;
	void ToStream(FILE* file) {
		std::string s = ToString();
		fprintf(file, "%s", ToString().c_str());
	}

	virtual SchedulePoint* Clone() = 0;

private:
	DECL_FIELD(bool, is_resolved)

};

/*
 * Represents a yield point
 */
class YieldPoint : public SchedulePoint {

public:
	YieldPoint();
	YieldPoint(std::string& source,
			std::string& label, unsigned int count = 1,
			SourceLocation* loc = NULL, SharedAccess* access = NULL,
			bool free_target = true, bool free_count = true);
	YieldPoint(Coroutine* source,
			std::string& label, unsigned int count = 1,
			SourceLocation* loc = NULL, SharedAccess* access = NULL,
			bool free_target = true, bool free_count = true);

	virtual ~YieldPoint() {
		if(access_ != NULL) {
			delete access_;
		}
		if(loc_ != NULL) {
			delete loc_;
		}
	}

	void update_access_loc(SharedAccess* access = NULL, SourceLocation* loc = NULL) {
		if(access_ != NULL && access_ != access) {
			delete access_;
		}
		if(loc_ != NULL && loc_ != loc) {
			delete loc_;
		}
		set_access(access);
		set_loc(loc);
	}

	virtual std::string ToString();

	virtual YieldPoint* AsYield() { return this; }

	virtual SchedulePoint* Clone();

	virtual point_t Coverage();

	virtual void Load(Serializer* serializer) {}
	virtual void Store(Serializer* serializer) {}

private:
	// permanent fields
	DECL_FIELD(Coroutine*, source) // can also point to std::string*
	DECL_FIELD(std::string, label)
	DECL_FIELD(unsigned int, count)
	DECL_FIELD(SourceLocation*, loc)
	DECL_FIELD(SharedAccess*, access)
	DECL_FIELD(bool, free_target) // can we choose another target at backtrack?
	DECL_FIELD(bool, free_count) // can we choose another count at backtrack?
	DECL_FIELD(TransferPoint*, prev) // if non-null, the transfer that leads to this yield

	friend class TransferPoint;
};

/********************************************************************************/

class TransferPoint : public SchedulePoint {

public:
	TransferPoint();
	TransferPoint(YieldPoint* point, Coroutine* target);
	TransferPoint(YieldPoint* point, std::string& target);
	virtual ~TransferPoint() {
		if(yield_ != NULL) {
			delete yield_;
		}
	}

	virtual void Load(Serializer* serializer);
	virtual void Store(Serializer* serializer);

	virtual std::string ToString();

	virtual YieldPoint* AsYield() { return safe_notnull(yield_); }
	virtual TransferPoint* AsTransfer() { return this; }

	virtual SchedulePoint* Clone();

	virtual bool IsResolved() { safe_assert(!is_resolved_ || yield_->is_resolved_); return is_resolved_; }

	bool ConsumeOnce();
	void ConsumeAll();

	void Reset();

	void make_backtrack_point() {
		safe_assert(IsResolved());
		safe_assert(target_ != NULL);
		safe_assert(enabled_.find(target_) != enabled_.end());
		safe_assert(done_.find(target_) != done_.end());

		target_ = NULL;
	}

	// if dpo_enabled then (backtrack \ done) == {} else (enabled \ done) == {}
	bool has_more_targets(bool dpor_enabled = true) {
		CoroutinePtrSet* members = dpor_enabled ? &backtrack_ : &enabled_;
		for(CoroutinePtrSet::iterator itr = members->begin(); itr != members->end(); ++itr) {
			Coroutine* co = (*itr);
			if(done_.find(co) == done_.end()) {
				safe_assert(enabled_.find(co) != enabled_.end());
				return true;
			}
		}
		return false;
	}

	// whether this is taken for the first time (means done is empty)
	inline bool is_first_transition() {
		return done_.empty();
	}

	virtual point_t Coverage() {
		return AsYield()->Coverage();
	}

private:
	DECL_FIELD(YieldPoint*, yield)
	DECL_FIELD(Coroutine*, target) // can also point to std::string*
	DECL_FIELD(unsigned int, rem_count)
	DECL_FIELD(YieldPoint*, next) // if non-null, the yield point this transfer enables

	// fields related to partial order reduction
	DECL_FIELD_REF(CoroutinePtrSet, enabled)
	DECL_FIELD_REF(CoroutinePtrSet, done) // targets tried before when taken
	DECL_FIELD_REF(CoroutinePtrSet, backtrack)
};

/********************************************************************************/

class ChoicePoint : public SchedulePoint {
public:
	ChoicePoint(Coroutine* source = Coroutine::Current())
	: source_(source) { is_resolved_ = true; }

	virtual ~ChoicePoint() {}

	ChoicePoint* AsChoice() { return this; }

	void SetAndConsumeCurrent();
	// return not null if there is a choice point in the schedule
	static ChoicePoint* GetCurrent(Coroutine* source = Coroutine::Current());

	// do not write choose for now
	std::string ToString() {
		std::stringstream s;
		s << "Choice point by" << (source_ != NULL ? source_->tid() : -1) ;
		return s.str();
	}

	virtual bool ChooseNext() = 0;
	virtual SchedulePoint* Clone() = 0;
	virtual void Load(Serializer* serializer) = 0;
	virtual void Store(Serializer* serializer) = 0;

private:
	DECL_FIELD(Coroutine*, source)
};

/********************************************************************************/

/*
 * Represents a list of schedule points
 */
class Schedule : public Serializable {

public:

	Schedule();
	Schedule(const char* filename, bool load = false);

	virtual ~Schedule();

	void SaveToFile(const char* filename = NULL, bool append = false);
	void LoadFromFile(const char* filename = NULL);
	void Load(Serializer* serializer);
	void Store(Serializer* serializer);

	void AddCurrent(SchedulePoint* point, bool consume);
	SchedulePoint* RemoveCurrent();
	bool HasCurrent();
	SchedulePoint* GetCurrent();
	void ConsumeCurrent();
	void SetCurrentToLast();
	void SetCurrentToFirst();
	size_t Size();

	void RemoveCurrentAndBeyond();

	void Clear();
	void Restart();

	void AddLast(SchedulePoint* point);
	SchedulePoint* RemoveLast();
	SchedulePoint* GetLast();

	virtual std::string ToString();
	virtual void ToStream(const char* filename = NULL, bool append = false);

	void ClearUntakenPoints();

	Schedule* Clone();

	Schedule & operator= (const Schedule& that);

protected:
	void delete_points();
	void extend_points(Schedule* that);

private:
	DECL_FIELD_REF(std::vector<SchedulePoint*>, points)
	DECL_FIELD(std::string, filename)
	DECL_FIELD(size_t, index)
	DECL_FIELD_REF(Coverage, coverage);

	friend class Scenario;
};

} // end namespace

#endif /* SCHEDULE_H_ */
