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

ExistsResult::ExistsResult(Schedule* schedule) {
	schedule_ = schedule;
//	coverage_ = *(schedule_->coverage());
}

ExistsResult::~ExistsResult() {
	if(schedule_ != NULL) {
		delete schedule_;
	}
}

std::string ExistsResult::ToString() {
	safe_assert(schedule_ != NULL);
	std::stringstream s;
	s << SuccessResult::ToString() << "\n";
	s << "Found successful execution.\n";
	s << statistics_.ToString() << "\n";

//		schedule_->ToStream(InWorkDir("schedule.out"));
//		s << "Schedule saved to schedule.out.\n";
//
//		s << "Total coverage:\n" << coverage_.ToString() << "\n";

	return s.str();
}

/*****************************************************************************/

ForallResult::~ForallResult() {
	for(std::vector<Schedule*>::iterator itr = schedules_.begin(); itr < schedules_.end(); ++itr) {
		safe_assert(*itr != NULL);
		delete *itr;
	}
}

void ForallResult::AddSchedule(Schedule* RESTRICT schedule) {
	num_all_schedules_++;
//	if(coverage_.AddAll(schedule->coverage())) {
//		schedules_.push_back(static_cast<Schedule*>(schedule));
//	} else {
//		delete schedule;
//	}
}

std::string ForallResult::ToString() {
	std::stringstream s;
	s << SuccessResult::ToString() << "\n";
	s << "Found " << num_all_schedules_ << " successful executions.\n";
	s << statistics_.ToString() << "\n";

//		s << "Found " << schedules_.size() << " coverage-increasing executions.\n";
//		bool append = false;
//		for(std::vector<Schedule*>::iterator itr = schedules_.begin(); itr < schedules_.end(); ++itr) {
//			Schedule* schedule = (*itr);
//			schedule->ToStream(InWorkDir("schedule.out"), append); append = true;
//		}
//		s << "Schedules saved to schedule.out.\n";
//
//		s << "Total coverage:\n" << coverage_.ToString() << "\n";

	return s.str();
}

/*****************************************************************************/

std::string SignalResult::ToString() {
	std::stringstream s;
	s << FailureResult::ToString() << "\n";
	s << "Signal received: " << signal_number_ << "\n";
	s << statistics_.ToString() << "\n";
	return s.str();
}

/*****************************************************************************/

AssertionViolationResult::AssertionViolationResult(AssertionViolationException* cause, Schedule* schedule) {
	cause_ = cause;
	schedule_ = safe_notnull(schedule);
//	coverage_ = *(schedule_->coverage());
}

AssertionViolationResult::~AssertionViolationResult() {
	if(cause_ != NULL) {
		delete cause_;
	}
	if(schedule_ != NULL) {
		delete schedule_;
	}
}

std::string AssertionViolationResult::ToString() {
	safe_assert(cause_ != NULL);
	safe_assert(schedule_ != NULL);
	std::stringstream s;
	s << FailureResult::ToString() << "\n";
	s << "Assertion Failure.\n";
	s << cause_->what() << "\n";
	s << statistics_.ToString() << "\n";

//		schedule_->ToStream(InWorkDir("schedule.out"));
//		s << "Schedule saved to schedule.out.\n";
//
//		s << "Total coverage:\n" << coverage_.ToString() << "\n";

	return s.str();
}


/*****************************************************************************/

RuntimeExceptionResult::RuntimeExceptionResult(std::exception* cause, Schedule* schedule) {
	cause_ = cause;
	schedule_ = safe_notnull(schedule);
//	coverage_ = *(schedule_->coverage());
}

RuntimeExceptionResult::~RuntimeExceptionResult() {
	if(cause_ != NULL) {
		delete cause_;
	}
	if(schedule_ != NULL) {
		delete schedule_;
	}
}

std::string RuntimeExceptionResult::ToString() {
	safe_assert(cause_ != NULL);
	safe_assert(schedule_ != NULL);
	std::stringstream s;
	s << FailureResult::ToString() << "\n";
	s << "Runtime Exception.";
	s << cause_->what() << "\n";
	s << statistics_.ToString() << "\n";

//		schedule_->ToStream(InWorkDir("schedule.out"));
//		s << "Schedule saved to schedule.out.\n";
//
//		s << "Total coverage:\n" << coverage_.ToString() << "\n";

	return s.str();
}

/*****************************************************************************/

NoFeasibleExecutionResult::NoFeasibleExecutionResult(Schedule* schedule) {
	schedule_ = safe_notnull(schedule);
//	coverage_ = *(schedule_->coverage());
}

NoFeasibleExecutionResult::~NoFeasibleExecutionResult() {
	if(schedule_ != NULL) {
		delete schedule_;
	}
}

std::string NoFeasibleExecutionResult::ToString() {
	safe_assert(schedule_ != NULL);
	std::stringstream s;
	s << FailureResult::ToString() << "\n";
	s << "No feasible execution found!\n";
	s << statistics_.ToString() << "\n";

//		schedule_->ToStream(InWorkDir("schedule.out"));
//		s << "Schedule saved to schedule.out.\n";
//
//		s << "Total coverage:\n" << coverage_.ToString() << "\n";

	return s.str();
}

/*****************************************************************************/

} // end namespace
