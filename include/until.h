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

#ifndef UNTIL_H_
#define UNTIL_H_

#include "common.h"
#include "api.h"
#include "schedule.h"

namespace concurrit {

class UntilCondition {
public:
	UntilCondition(){}
	virtual ~UntilCondition(){}

	virtual bool Check(SchedulePoint* point) = 0;
};

/********************************************************************************/

class UntilLabelCondition : public UntilCondition {
public:
	explicit UntilLabelCondition(const std::string& label) : UntilCondition() {
		label_ = label;
	}
	~UntilLabelCondition(){}

	virtual bool Check(SchedulePoint* point) {
		MYLOG(2) << "Checking labels " << this->label_ << " and " << point->AsYield()->label();
		return this->label_ == point->AsYield()->label();
	}

private:
	DECL_FIELD(std::string, label)
};

/********************************************************************************/

class UntilEndCondition : public UntilLabelCondition {
public:
	UntilEndCondition() : UntilLabelCondition(ENDING_LABEL) {}
	~UntilEndCondition(){}
};

/********************************************************************************/

class UntilStarCondition : public UntilCondition {
public:
	UntilStarCondition() : UntilCondition() {}
	~UntilStarCondition(){}

	virtual bool Check(SchedulePoint* point) {
		MYLOG(2) << "Checking Until STAR!";
		return point->IsTransfer() || point->AsYield()->label() == ENDING_LABEL ;
	}
};

/********************************************************************************/

class UntilFirstCondition : public UntilCondition {
public:
	UntilFirstCondition() : UntilCondition() {}
	~UntilFirstCondition(){}

	bool Check(SchedulePoint* point) {
		MYLOG(2) << "Checking Until FIRST!";
		return true;
	}
};

/********************************************************************************/

// set of conditions for transfers from main
class TransferCriteria {
public:
	TransferCriteria() {
		Reset();
	}
	~TransferCriteria() {
		Reset();
	}

	void Reset() {
		if(until_ != NULL && until_ != &until_star_ && until_ != &until_first_ && until_ != &until_end_) {
			delete until_;
		}
		until_ = until_star();
		except_.clear();
	}

	void UntilFirst() {
		Until(until_first());
	}

	void UntilEnd() {
		Until(until_end());
	}

	void UntilStar() {
		Until(until_star());
	}

	void Until(UntilCondition* cond) {
		safe_assert(cond != NULL);
		until_ = cond;
	}

	void Until(const std::string& label) {
		Until(new UntilLabelCondition(label));
	}

	void Except(Coroutine* t) {
		if(t != NULL) {
			except_.insert(t);
		}
	}


private:
	DECL_FIELD(UntilCondition*, until)
	DECL_FIELD_REF(CoroutinePtrSet, except)

	DECL_STATIC_FIELD_REF(UntilStarCondition, until_star)
	DECL_STATIC_FIELD_REF(UntilFirstCondition, until_first)
	DECL_STATIC_FIELD_REF(UntilEndCondition, until_end)
};

/********************************************************************************/


} // end namespace

#endif /* UNTIL_H_ */
