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

#ifndef RESULT_H_
#define RESULT_H_

#include "common.h"

namespace concurrit {

class Result {
public:
	Result() {}
	virtual ~Result() {}

	virtual bool IsSuccess() = 0;
	virtual std::string ToString() = 0;

private:
//	DECL_FIELD_REF(Coverage, coverage)
	DECL_FIELD(Statistics, statistics)
};

/********************************************************************************/

class SuccessResult : public Result {
public:
	SuccessResult() : Result() {}
	virtual ~SuccessResult() {}

	virtual bool IsSuccess() { return true; }

	virtual std::string ToString() {
		return std::string("SUCCESS.");
	}
};

/********************************************************************************/

class FailureResult : public Result {
public:
	FailureResult() : Result() {}
	virtual ~FailureResult() {}

	virtual bool IsSuccess() { return false; }
	virtual std::string ToString() {
		return std::string("FAILURE.");
	}
};

/********************************************************************************/

class ExistsResult : public SuccessResult {
public:
	explicit ExistsResult(Schedule* schedule);

	virtual ~ExistsResult();

	std::string ToString();

private:
	DECL_FIELD(Schedule*, schedule)
};

/********************************************************************************/

class ForallResult : public SuccessResult {
public:
	ForallResult() : num_all_schedules_(0) {}

	~ForallResult();

	void AddSchedule(Schedule* RESTRICT schedule);

	std::string ToString();

private:
	DECL_FIELD(std::vector<Schedule*>, schedules)
	DECL_FIELD(unsigned int, num_all_schedules)
};

/********************************************************************************/

class SignalResult : public FailureResult {
public:
	explicit SignalResult(int signal_number) :signal_number_(signal_number) {
		safe_assert(signal_number > 0);
	}

	~SignalResult() {}

	std::string ToString();

private:
	DECL_FIELD(int, signal_number)
};

/********************************************************************************/

class AssertionViolationResult : public FailureResult {
public:
	explicit AssertionViolationResult(AssertionViolationException* cause, Schedule* schedule);

	virtual ~AssertionViolationResult();


	std::string ToString();

private:
	DECL_FIELD(AssertionViolationException*, cause)
	DECL_FIELD(Schedule*, schedule)
};

/********************************************************************************/

class RuntimeExceptionResult : public FailureResult {
public:
	explicit RuntimeExceptionResult(std::exception* cause, Schedule* schedule);

	virtual ~RuntimeExceptionResult();

	std::string ToString();

private:
	DECL_FIELD(std::exception*, cause)
	DECL_FIELD(Schedule*, schedule)
};

/********************************************************************************/

class NoFeasibleExecutionResult : public FailureResult {
public:
	explicit NoFeasibleExecutionResult(Schedule* schedule);

	virtual ~NoFeasibleExecutionResult();

	std::string ToString();

private:
	DECL_FIELD(Schedule*, schedule)
};

/********************************************************************************/

} // end namespace

#endif /* RESULT_H_ */
