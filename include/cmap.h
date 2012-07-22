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

#ifndef CMAP_H_
#define CMAP_H_

#include "common.h"
#include "thread.h"

namespace concurrit {

template<typename K, typename T>
class ConcurrentMap : public std::map<K, T> {
	typedef std::map<K, T> super;
public:
	ConcurrentMap() : super() {}
	~ConcurrentMap() {}

	bool contains(const K& key) {
		ScopeRLock m(&rwlock_);
		return super::find(key) != super::end();
	}

	bool find(const K& key, T* value = NULL) {
		ScopeRLock m(&rwlock_);
		typename super::iterator itr = super::find(key);
		if(itr != super::end()) {
			if(value != NULL) {
				(*value) = itr->second;
			}
			return true;
		}
		return false;
	}

	void insert(const K& key, const T& value) {
		ScopeWLock m(&rwlock_);
		(*this)[key] = value;
	}

	bool remove(const K& key) {
		ScopeWLock m(&rwlock_);
		typename super::iterator itr = super::find(key);
		if(itr != super::end()) {
			super::erase(itr);
			return true;
		}
		return false;
	}

	//=======================================================//

	inline void set(const K& key, const T& value) {
		insert(key, value);
	}

	inline bool isset(const K& key) {
		return contains(key);
	}

	T get(const K& key) {
		T value;
		if(find(key, &value)) {
			return value;
		}
		safe_fail("ConcurrentMap: get must be called for existing mappings!")
	}

private:
	DECL_FIELD(RWLock, rwlock)
};

} // end namespace

#endif /* CMAP_H_ */
