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

#ifndef PREDICATE_H_
#define PREDICATE_H_

#include "common.h"

namespace concurrit {

/********************************************************************************/

template<unsigned N, class T>
class PredicateBase {
public:
	typedef std::map<std::string, T> ArgToValueMap;

	explicit PredicateBase(T default_value) : default_value_(default_value) {}
	virtual ~PredicateBase(){}

	T get (std::string& key) {
		typename ArgToValueMap::iterator itr = map_.find(key);
		if(itr == map_.end()) {
			return default_value_;
		}
		return itr->second;
	}

	void set(std::string& key, const T& value) {
		if(value == default_value_) {
			map_.erase(key);
		} else {
			map_[key] = value;
		}
	}

	unsigned arity() {
		return N;
	}

	void Reset() {
		map_.clear();
	}

private:
	DECL_FIELD(ArgToValueMap, map)
	DECL_FIELD(T, default_value)

};

/********************************************************************************/

template<unsigned N, class T>
class PredArgs {
public:
	explicit PredArgs(PredicateBase<N, T>* pred) : pred_(pred), key_(""), count_(0) {
		safe_assert(pred != NULL);
	}
	~PredArgs() {}

	template<typename K>
	PredArgs<N, T>& operator << (const K& v) {
		safe_assert(count_ < N);
		count_++;
		key_ << "*" << v << "#";
		return *this;
	}

	T operator () () {
		safe_assert(count_ == N);
		return pred_->get(key_.str());
	}

	PredArgs<N, T>& operator = (const T& value) {
		safe_assert(count_ == N);
		std::string key = key_.str();
		pred_->set(key, value);
		return *this;
	}

	unsigned arity() {
		return N;
	}

private:
	PredicateBase<N, T>* pred_;
	DECL_FIELD(std::stringstream, key)
	DECL_FIELD(unsigned, count)
};

/********************************************************************************/

template<unsigned N>
class Predicate : public PredicateBase<N, bool> {
public:
	explicit Predicate(bool default_value = false) : PredicateBase<N, bool>(default_value) {}
	virtual ~Predicate(){}

	template<typename T>
	PredArgs<N, bool>& operator << (const T& v) {
		safe_assert(N > 0);
		return PredArgs<N, bool>(this) << v;
	}
};

/********************************************************************************/

template<>
class Predicate<0> : public PredicateBase<0, bool> {
public:
	Predicate(bool default_value = false) : PredicateBase<0, bool>(default_value) {}
	virtual ~Predicate(){}

	template<typename T>
	PredArgs<0, bool>& operator << (const T& v) {
		safe_assert(false);
		return PredArgs<0, bool>(this) << v;
	}

	bool operator () () {
		return default_value_;
	}

	Predicate<0>& operator = (const bool& value) {
		default_value_ = value;
		return *this;
	}
};

/********************************************************************************/

} // namespace


#endif /* PREDICATE_H_ */
