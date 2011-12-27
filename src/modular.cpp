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

#include "counit.h"

namespace counit {

void ThreadModularScenario::OnAccess(Coroutine* current, SharedAccess* access) {
	super::OnAccess(current, access);

	// trace the access
	// if a read, update the current state
	// if a write, move to a new state
	env_trace_.OnAccess(current, access);
}

void ThreadModularScenario::Restart() {
	super::Restart();

	// restart memory trace
	env_trace_.Restart();
}

void ThreadModularScenario::AfterRunOnce() {
	super::AfterRunOnce();

	// print the memory trace:
	printf("Memory trace:\n%s\n", env_trace_.ToString().c_str());

	// add memory trace to env graph
	env_graph_.Update(&env_trace_);
}

void* env_thread_function(void* p) {
	CHECK(p != NULL);
	EnvTrace* env_trace = static_cast<EnvTrace*>(p);
	while(true) {
		VLOG(2) << "Running env thread context";

		SchedulePoint* main_point = Coroutine::Current()->yield_point();
		if(main_point != NULL) {
			main_point = main_point->AsYield()->prev();

			safe_assert(main_point != NULL && main_point->IsTransfer());
			safe_assert(main_point->AsYield()->source()->IsMain());

			Coroutine* source = NULL;
			SharedAccess* access = NULL;
			SchedulePoint* point = main_point->AsYield()->prev();
			if(point != NULL) {
				safe_assert(point->IsTransfer());
				source = point->AsYield()->source();
				safe_assert(!source->IsMain());

				// handle the environment transitions here
				access = point->AsYield()->access();
			}

			VLOG(2) << "env_trace is taking a step";
			if(!env_trace->Step(source, access)) {
				break;
			}
		}
		// force yield after each step
		FORCE_YIELD("ENV", NULL);
	} // end while
	return NULL;
}



// this is a API call, given a set of threads, makes a modular check
void ThreadModularScenario::TestCase() {
	TEST_FORALL(); // we chech all possible executions

	// add environment thread to the group
	Coroutine* env_co = CREATE_THREAD("ENV", env_thread_function, &env_trace_);

	// add a choice point for choosing which member to run (or reuse it if already exists)
	MemberChoicePoint* member_choice = NULL;
	ChoicePoint* choice_point = ChoicePoint::GetCurrent();
	if(choice_point != NULL) {
		VLOG(2) << SC_TITLE << "Reusing choice point";

		member_choice = ASINSTANCEOF(choice_point, MemberChoicePoint*);
		schedule_->ConsumeCurrent();
	} else {
		VLOG(2) << SC_TITLE << "Creating member_choice";

		CoroutinePtrSet members = group_.GetMemberSet();
		// remove env thread
		members.erase(members.find(env_co));
		member_choice = new MemberChoicePoint(members);
		member_choice->SetCurrent();
	}

	Coroutine* co = member_choice->GetNext();
	if(co == NULL) {
		VLOG(2) << SC_TITLE << "All members were used, so backtracking";
		// no more members, so backtrack (and end the execution)
		TRIGGER_BACKTRACK();
	}

	safe_assert(co != env_co);

	// run one member at a time concurrently with env_thread
	printf("Modular check with coroutine %s\n", co->name().c_str());
	{ WITH(co, env_co);

		UNTIL_ALL_END {
			UNTIL_STAR()->TRANSFER_STAR();
		}

	}
}

/********************************************************************************/

bool EnvTrace::Step(Coroutine* current, SharedAccess* access) {
	// decide whether to stay in the same node or transit to another node
	// TODO(elmas): stays for now
	return false;
}

void EnvTrace::OnAccess(Coroutine* current, SharedAccess* access) {
	EnvNodePtr node;
	if(access->is_read()) {
		// check if any state exists
		if(nodes_.empty()) {
			node = env_graph_->MakeNewNode();
			nodes_.push_back(node);
		} else {
			node = nodes_.back();
		}
	} else {
		node = nodes_.empty() ? env_graph_->MakeNewNode() : env_graph_->MakeNewNode(nodes_.back());
		nodes_.push_back(node);
	}
	// update the node
	node->Update(current, access->cell());

	// make node the current node of the trace
	current_ = node;
}

void EnvTrace::Restart(EnvGraph* g /*=NULL*/) {
	// delete all states
	delete_nodes();
	if(env_graph_ == NULL || g != NULL) {
		env_graph_ = g;
	}
	safe_assert(env_graph_ != NULL);
	current_ = env_graph_->start_node();
	nodes_.push_back(current_);
}

void EnvTrace::delete_nodes() {
	// nodes are shared pointers, so do not delete them, just clear the list
	// shared_ptr takes care of the rest
	nodes_.clear();
}

std::string EnvTrace::ToString() {
	std::stringstream s;

	int i = 1;
	for(EnvNodePtrList::iterator itr = nodes_.begin(); itr < nodes_.end(); ++itr) {
		EnvNodePtr node = (*itr);
		s << i << " : " << node->ToString() << "\n";
		s << "********************\n";
		++i;
	}

	return s.str();
}

/********************************************************************************/

void EnvNode::Update(Coroutine* current, MemoryCellBase* cell) {
	// update globals
	globals_.Update(cell);
}

void EnvNode::AddEdge(EnvNodePtr node) {
	edges_.insert(node);
}

std::string EnvNode::ToString() {
	std::stringstream s;

	s << "MemoryMap: " << globals_.ToString();

	return s.str();
}

/********************************************************************************/

void MemoryMap::Update(MemoryMap* other) {
	for(CellMap::iterator itr = other->memToCell_.begin(); itr != other->memToCell_.end(); ++itr) {
		MemoryCellBase* cell = itr->second;
		this->Update(cell);
	}
}

void MemoryMap::Update(MemoryCellBase* cell) {
	ADDRINT mem = cell->int_address();
	CellMap::iterator itr = memToCell_.find(mem);
	if(itr != memToCell_.end()) {
		itr->second->update_value(cell);
	} else {
		memToCell_[mem] = cell->Clone();
	}
}

std::string MemoryMap::ToString() {
	std::stringstream s;

	for(CellMap::iterator itr = memToCell_.begin(); itr != memToCell_.end(); ++itr) {
		s << itr->second->ToString() << " ";
	}

	return s.str();
}

void MemoryMap::delete_cells() {
	for(CellMap::iterator itr = memToCell_.begin(); itr != memToCell_.end(); ++itr) {
		MemoryCellBase* cell = itr->second;
		delete cell;
		memToCell_.erase(itr);
	}
	safe_assert(memToCell_.empty());
}

MemoryMap* MemoryMap::Clone() {
	MemoryMap* other = new MemoryMap();
	for(CellMap::iterator itr = memToCell_.begin(); itr != memToCell_.end(); ++itr) {
		MemoryCellBase* cell = itr->second;
		other->Update(cell);
	}
	return other;
}

/********************************************************************************/

EnvNodePtr EnvGraph::MakeNewNode(EnvNodePtr existing) {
	EnvNodePtr node = EnvNodePtr(new EnvNode());
	// update globals from the existing node
	if(existing != NULL) {
		node->globals()->Update(existing->globals());
	}
	return node;
}

void EnvGraph::AddNode(EnvNodePtr node) {
	// we have at least start node
	safe_assert(start_node_ != NULL);
	nodes_.insert(node);
}

void EnvGraph::Update(EnvTrace* trace) {
	EnvNodePtr prev;
	// traverse the trace, and add new nodes and connections between nodes
	EnvNodePtrList* nodes = trace->nodes();
	for(EnvNodePtrList::iterator itr = nodes->begin(); itr < nodes->end(); ++itr) {
		EnvNodePtr node = (*itr);
		if(nodes_.find(node) == nodes_.end()) {
			// add the node
			AddNode(node);
		}
		if(prev != NULL) {
			// add edge from prev to node
			prev->AddEdge(node);
		}
		prev = node;
	}
}

void EnvGraph::delete_nodes() {
	// nodes are shared pointers, so do not delete them, just clear the list
	// shared_ptr takes care of the rest
	nodes_.clear();
}


} // end namespace
