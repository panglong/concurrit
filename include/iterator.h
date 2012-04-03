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


#ifndef ITERATOR_H_
#define ITERATOR_H_

#include "common.h"

namespace concurrit {

template<typename T>
class IteratorState {
public:
	typedef std::vector<T> vectorT;
	typedef std::set<T> setT;

	IteratorState(const vectorT& x)
	: vector_(x), current_(0) {}

	IteratorState(vectorT* x)
	: vector_(*x), current_(0) {}

	IteratorState(const setT& x)
	: current_(0) {
		for(typename setT::iterator itr = x.begin(); itr != x.end(); ++itr) {
			vector_.push_back(*itr);
		}
	}

	IteratorState(setT* x)
	: current_(0) {
		for(typename setT::iterator itr = x->begin(); itr != x->end(); ++itr) {
			vector_.push_back(*itr);
		}
	}

	T current() {
		safe_assert(has_current());
		return vector_[current_];
	}

	bool has_current() {
		return 0 <= current_ && current_ < int(vector_.size());
	}

	bool next() {
		safe_assert(has_current());
		current_++;
		return has_current();
	}

	bool prev() {
		safe_assert(has_current());
		current_--;
		return has_current();
	}

private:
	vectorT vector_;
	int current_;
};

} // end namespace

#endif /* ITERATOR_H_ */
