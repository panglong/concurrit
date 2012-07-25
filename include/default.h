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

#ifndef DEFAULT_H_
#define DEFAULT_H_

#include "common.h"
#include "scenario.h"
#include "api.h"

namespace concurrit {

// scenario defining default search policies
class DefaultScenario : public Scenario {
public:
	explicit DefaultScenario(const char* name) : Scenario(name) {}
	virtual ~DefaultScenario() {}

	/* ============================================================================= */

//	void NDSequentialSearch(TransitionPredicatePtr select_criteria = PTRUE) {
//
//		WHILE_DTSTAR {
//			TVAR(t);
//			CHOOSE_THREAD_BACKTRACK(t, select_criteria, "Forall thread");
//			RUN_THREAD_UNTIL(t, ENDS(), __);
//		}
//	}
//
//	/* ============================================================================= */
//
//	void NDConcurrentSearch(TransitionPredicatePtr select_criteria = PTRUE,
//							TransitionPredicatePtr trans_criteria = PTRUE) {
//		WHILE_DTSTAR {
//			FORALL(t, select_criteria, "Forall thread");
//			RUN_THREAD_UNTIL(t, trans_criteria, __);
//		}
//	}
//
//	/* ============================================================================= */
//
//	void NDConcurrentContextBoundedSearch(unsigned bound,
//										  TransitionPredicatePtr select_criteria = PTRUE,
//										  TransitionPredicatePtr trans_criteria = PTRUE) {
//		CHECK(bound > 0) << "Context bound must be > 1!";
//
//		TVAR(t_old);
//		for(unsigned i = 0; i < bound && !ALL_ENDED; ) {
//
//			CHOOSE_THREAD_BACKTRACK(t, select_criteria && (TID != t_old), "Forall thread");
//			WHILE_DTSTAR {
//				RUN_THREAD_UNTIL(t, trans_criteria, __);
//			}
//			if(!HAS_ENDED(t)) {
//				++i;
//			}
//			t_old << t;
//		}
//
//		NDSequentialSearch();
//	}

	/* ============================================================================= */

	/* ============================================================================= */


	/* ============================================================================= */

};



} // end namespace

#endif /* DEFAULT_H_ */
