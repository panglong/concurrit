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

#ifndef SUITE_H_
#define SUITE_H_

#include "common.h"
#include "result.h"
#include "scenario.h"

namespace concurrit {

class Suite {
public:
	Suite() {}
	virtual ~Suite(){}

	void AddScenario(Scenario* scenario);
	void RemoveScenario(Scenario* scenario);
	void RemoveScenario(const std::string& name);

	std::map<std::string, Result*> RunScenarios();
	void RunAll();

private:
	DECL_FIELD_REF(std::vector<Scenario*>, scenarios)
	DECL_FIELD_REF(Coverage, coverage)
};

/********************************************************************************/

class Suite;
template<class T>
class StaticSuiteAdder {
public:
	StaticSuiteAdder(Suite* suite) : suite_(suite) {
		scenario_ = new T();
		suite_->AddScenario(scenario_);
	}
	~StaticSuiteAdder() {
		safe_assert(scenario_ != NULL);
		suite_->RemoveScenario(scenario_);
	}
private:
	DECL_FIELD(Suite*, suite)
	DECL_FIELD(Scenario*, scenario)
};

} // end namespace

#endif /* SUITE_H_ */
