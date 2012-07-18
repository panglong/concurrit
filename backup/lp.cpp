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

Coverage* tuple_t::Derive() {
	safe_assert(len() == 2);

	Coverage* cov = new Coverage();

	point_t points[] = {points_[1], points_[0]};
	tuple_t tuple(2, points);
	cov->AddTuple(tuple);

	return cov;
}

/************************************************************************/

Coverage* Coverage::Derive() {
	Coverage* cov = new Coverage();

	for(TupleSet::iterator itr = tuples_.begin(); itr != tuples_.end(); ++itr) {
		tuple_t tuple = (*itr);
		cov->AddAll(tuple.Derive());
	}

	return cov;
}

/************************************************************************/

void VCTracker::UpdateBacktrackSets(SchedulePoint* currentPoint) {
	SharedAccess* currentAccess = currentPoint->AsYield()->access();
	if(currentAccess == NULL) return;

	ADDRINT mem = currentAccess->mem();

	Coroutine* currentSource = currentPoint->AsYield()->source();
	safe_assert(currentSource != NULL);
	THREADID tid = currentSource->tid();
	safe_assert(tid > 0);

	// find immediate races
	std::set<SchedulePoint*>* accesses = GetLastAccesses(mem); // may return NULL if there are no last accesses

	VC vcTid = currentSource->vc();

	bool is_conflicting = false;
	if(accesses != NULL && !accesses->empty()) {

		// iterate over last accesses
		for(std::set<SchedulePoint*>::iterator itr = accesses->begin(); itr != accesses->end(); itr++) {
			SchedulePoint* point = (*itr);
			SharedAccess* access = point->AsYield()->access();
			safe_assert(access != NULL);
			safe_assert(access->mem() == mem);

			if(CONFLICTING(access->type(), currentAccess->type())) {
				is_conflicting = true;
			} else {
				safe_assert(!is_conflicting);
				break;
			}

			// TODO(elmas): try to remove this check, because it seems that an access is handled here before it happens
			// there is something wrong with overwriting the access (happens with loops).
			if(access->time() == 0) {
				// in this case we should be visiting the same thread
				safe_assert(point->AsYield()->source() == currentPoint->AsYield()->source());
				continue;
			}

			safe_assert(BETWEEN(1, access->time(), currentTime_-1));

			// check immediate race
			THREADID lastTid = point->AsYield()->source()->tid();
			safe_assert(lastTid > 0);
			if(lastTid != tid) {
				if(!(access->time() <= vc_get(vcTid, lastTid))) {
					// immediate race between access and current access
					// we update the backtrack set of the previous transfer here
					TransferPoint* transfer = point->AsYield()->prev();
					if(transfer != NULL && transfer->AsYield()->free_target()) {
						MYLOG(2) << "VCTracker: Adding backtrack point";
						CoroutinePtrSet* enabled = transfer->enabled();
						if(enabled->find(currentSource) != enabled->end()) {
							// add only currentSource to backtrack
							transfer->backtrack()->insert(currentSource);
						} else {
							// add all enabled to backtrack
							transfer->backtrack()->insert(enabled->begin(), enabled->end());
						}
					}
				}
			}
		}
	}
}

/************************************************************************/

void VCTracker::HandleImmediateRace(SchedulePoint* point, SchedulePoint* currentPoint, Coverage* coverage) {

	MYLOG(2) << "Immediate race between "
			<< point->AsYield()->access()->ToString() << " and "
			<< currentPoint->AsYield()->access()->ToString();

	// update coverage
	point_t labels[] = {point->AsYield()->Coverage(), currentPoint->AsYield()->Coverage()};
	tuple_t tuple(2, labels);

	coverage->AddTuple(tuple);
}

void VCTracker::OnAccess(SchedulePoint* currentPoint, Coverage* coverage) {
	SharedAccess* currentAccess = currentPoint->AsYield()->access();
	if(currentAccess == NULL) return;

	ADDRINT mem = currentAccess->mem();

	Coroutine* currentSource = currentPoint->AsYield()->source();
	safe_assert(currentSource != NULL);
	THREADID tid = currentSource->tid();
	safe_assert(tid > 0);

	// find immediate races
	std::set<SchedulePoint*>* accesses = GetLastAccesses(mem, true); // create if absent
	safe_assert(accesses != NULL);

	VC vcTid = currentSource->vc();

	bool is_conflicting = false;
	if(!accesses->empty()) {

		// iterate over last accesses
		for(std::set<SchedulePoint*>::iterator itr = accesses->begin(); itr != accesses->end(); itr++) {
			SchedulePoint* point = (*itr);
			SharedAccess* access = point->AsYield()->access();
			safe_assert(access != NULL);
			safe_assert(access->mem() == mem);

			if(CONFLICTING(access->type(), currentAccess->type())) {
				is_conflicting = true;
			} else {
				safe_assert(!is_conflicting);
				break;
			}

			// TODO(elmas): try to remove this check, because it seems that an access is handled here before it happens
			// there is something wrong with overwriting the access (happens with loops).
			if(access->time() == 0) {
				// in this case we should be visiting the same thread
				safe_assert(point->AsYield()->source() == currentPoint->AsYield()->source());
				continue;
			}

			safe_assert(BETWEEN(1, access->time(), currentTime_-1));

			// check immediate race
			THREADID lastTid = point->AsYield()->source()->tid();
			safe_assert(lastTid > 0);
			if(lastTid != tid) {
				if(!(access->time() <= vc_get(vcTid, lastTid))) {
					// immediate race between access and current access
					HandleImmediateRace(point, currentPoint, coverage);
				}
			}
		}
		if(is_conflicting) {
			accesses->clear();
		}
	} else {
		is_conflicting = true;
	}

	// update vector clocks
	VC vcMem = vc_get_vc(memToVC_, mem);

	if(is_conflicting) {
		// acquire
		VC vcNew = vc_cup(vcMem, vcTid);
		vcMem = vcNew;

		// release
		vc_set(vcNew, tid, currentTime_);
		vcTid = vcNew;

	} else {
		// release
		vc_set(vcTid, tid, currentTime_);
		//vc_set(vcMem, tid, currentTime_);
	}

	currentSource->set_vc(vcTid);
	vc_set_vc(memToVC_, mem, vcMem);

	currentAccess->set_time(currentTime_);

	accesses->insert(currentPoint);

	currentTime_++;
}


std::set<SchedulePoint*>* VCTracker::GetLastAccesses(ADDRINT mem, bool createIfAbsent /*= false*/) {
	std::set<SchedulePoint*>* accesses = NULL;
	std::map<ADDRINT, std::set<SchedulePoint*>*>::iterator itr = memToLastAccess_.find(mem);
	if(itr != memToLastAccess_.end()) {
		accesses = itr->second;
		safe_assert(accesses != NULL);
	} else if(createIfAbsent) {
		accesses = new std::set<SchedulePoint*>();
		memToLastAccess_[mem] = accesses;
	}
	return accesses;
}

}
