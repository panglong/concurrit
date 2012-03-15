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


#include "common.h"

#ifndef YIELD_H_
#define YIELD_H_

namespace concurrit {

class Scenario;
class CoroutineGroup;
class Coroutine;
class SourceLocation;
class SharedAccess;

#define YIELD_SIGNATURE \
SchedulePoint* Yield(Scenario* scenario, \
					 CoroutineGroup* group, \
					 Coroutine* source, \
					 Coroutine* target, \
					 std::string& label, \
					 SourceLocation* loc, \
					 SharedAccess* access)


// interface for yield implementors
// default implementation is provided by Scenario
class YieldImpl {
public:
	YieldImpl() {}
	virtual ~YieldImpl() {}

	virtual YIELD_SIGNATURE = 0;
};

/********************************************************************************/

// struct storing arguments to a yield call
struct YieldCall {
	Scenario* scenario_;
	CoroutineGroup* group_;
	Coroutine* source_;
	Coroutine* target_;
	std::string label_;
	SourceLocation* loc_;
	SharedAccess* access_;

	YieldCall(Scenario* scenario,
			 CoroutineGroup* group,
			 Coroutine* source,
			 Coroutine* target,
			 std::string& label,
			 SourceLocation* loc,
			 SharedAccess* access)
	 : scenario_(scenario),
	   group_(group),
	   source_(source),
	   target_(target),
	   label_(label),
	   loc_(loc),
	   access_(access) {}
};



} // end namespace

#endif /* YIELD_H_ */
