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

namespace counit {

void Suite::AddScenario(Scenario* scenario) {
	safe_assert(scenario != NULL);
	scenarios_.push_back(scenario);
}

std::map<std::string, Result*> Suite::RunScenarios() {
	std::map<std::string, Result*> results;

	coverage_.Clear(); //  will accumulate this
	for(std::vector<Scenario*>::iterator itr = scenarios_.begin(); itr < scenarios_.end(); ++itr) {
		Scenario* scenario = (*itr);
		printf("Running scenario %s\n", scenario->name());
		Result* result = scenario->Explore();
		safe_assert(result != NULL);
		// accumulate coverage
		if(result->IsSuccess()) {
			coverage_.AddAll(static_cast<SuccessResult*>(result)->coverage());
		}
		results[scenario->name()] = result;
		printf("Done with scenario %s. %s\n", scenario->name(), (result->IsSuccess() ? "SUCCESS" : "FAILURE"));
	}
	return results;
}

void Suite::RunAll() {
	std::map<std::string, Result*> results = RunScenarios();

	std::stringstream s;
	int num_success = 0, num_failure = 0;
	// count success
	for(std::map<std::string, Result*>::iterator itr = results.begin(); itr != results.end(); ++itr) {
		Result* result = itr->second;
		if(result->IsSuccess()) ++num_success; else ++num_failure;
		s << "\nScenario: " << itr->first << "\n";
		s << result->ToString() << "\n";
		delete result;
	}

	// print statistics
	printf("\nSuite ended.\n");
	printf("%d succeeded. %d failed.\n", num_success, num_failure);

	printf("\nResults:");
	printf("%s", s.str().c_str());

	printf("Total coverage:\n%s\n", coverage_.ToString().c_str());
}

} // end namespace


