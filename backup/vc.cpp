
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

 VC vc_get_vc(VCMAP& m, ADDRINT k) {
	VCMAP::iterator it = m.find(k);
	if(it != m.end()) {
		return it->second;
	}
	return VC();
}
 void vc_set_vc(VCMAP& m, ADDRINT k, VC vc) {
	m[k] = vc;
}

void vc_clear(VC& v) {
	v.clear();
}

vctime_t vc_get(VC& vc, THREADID t) {
	VC::iterator it = vc.find(t);
	if(it != vc.end()) {
		vctime_t c = it->second;
		safe_assert(c > 0);
		return c;
	}
	return 0; // rest is 0
}

void vc_set(VC& vc, THREADID t, vctime_t c) {
	if(c > 0) {
		vc[t] = c;
	} else {
		vc.erase(t);
	}
}

void vc_inc(VC& vc, THREADID t) {
	VC _vc_ = vc;
	vctime_t c = vc_get(_vc_, t);
	_vc_[t] = c + 1;
}

VC vc_cup(VC& vc1, VC& vc2) {
	VC _vc_ = vc1;
	for (VC::iterator it=vc2.begin() ; it != vc2.end(); it++ ) {
		THREADID t = it->first;
		vctime_t c1 = vc_get(_vc_, t);
		vctime_t c2 = it->second;
		if(c2 > c1) {
			_vc_[t] = c2;
		}
	}
	return _vc_;
}

bool vc_leq(VC& vc1, VC& vc2, THREADID t) {
	vctime_t c1 = vc_get(vc1, t);
	vctime_t c2 = vc_get(vc2, t);
	return c1 <= c2;
}

bool vc_leq_all(VC& vc1, VC& vc2) {
	for (VC::iterator it=vc2.begin() ; it != vc2.end(); it++ ) {
		THREADID t = it->first;
		vctime_t c1 = vc_get(vc1, t);
		vctime_t c2 = it->second;
		if(c1 > c2) {
			return false;
		}
	}
	return true;
}

} // end namespace

