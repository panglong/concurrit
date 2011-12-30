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

/********************************************************************************/

void ThreadModularScenario::OnAccess(Coroutine* current, SharedAccess* access) {
	super::OnAccess(current, access);

	// trace the access
	// if a read, update the current state
	// if a write, move to a new state
	env_trace_.OnAccess(current, access);
}

/********************************************************************************/

void ThreadModularScenario::Restart() {
	super::Restart();

	// restart memory trace
	env_trace_.Restart();
}

/********************************************************************************/

void ThreadModularScenario::AfterRunOnce() {
	super::AfterRunOnce();

	// print the memory trace:
	printf("Memory trace:\n%s\n", env_trace_.ToString().c_str());

	// add memory trace to env graph
	env_graph_.Update(&env_trace_);
}

/********************************************************************************/

void* env_thread_function(void* p) {
	CHECK(p != NULL);
	EnvTrace* env_trace = static_cast<EnvTrace*>(p);
	for(;true;) {
		VLOG(2) << "Running env thread context";

		Coroutine* current = Coroutine::Current();
		safe_assert(!current->IsMain());

		SchedulePoint* main_point = current->yield_point();
		if(main_point != NULL) {
			main_point = main_point->AsYield()->prev();

			safe_assert(main_point != NULL && main_point->IsTransfer());
			safe_assert(main_point->AsYield()->source()->IsMain());

//			Coroutine* source = NULL;
//			SharedAccess* access = NULL;
//			SchedulePoint* point = main_point->AsYield()->prev();
//			if(point != NULL) {
//				safe_assert(point->IsTransfer());
//				source = point->AsYield()->source();
//				safe_assert(!source->IsMain());
//
//				// handle the environment transitions here
//				access = point->AsYield()->access();
//			}

			VLOG(2) << "env_trace is taking a step";

			// add a choice point for choosing which member to run (or reuse it if already exists)
			EnvChoicePoint* env_choice = NULL;
			ChoicePoint* choice_point = ChoicePoint::GetCurrent();
			if(choice_point != NULL) {
				VLOG(2) << "Reusing env choice point";

				env_choice = ASINSTANCEOF(choice_point, EnvChoicePoint*);
				safe_assert(env_choice->GetNext() != NULL);

				// check if current nodes of trace and the choice point are the same
				if(env_trace->current_node() != env_choice->current_node()) {
					TRIGGER_BACKTRACK();
				}
				// set the next current node of the trace
				env_trace->set_current_node(env_choice->GetNext());
			} else {
				VLOG(2) << "Creating new env choice point";

				// first taking of this transition
				// gets the current node of env_trace as the current node
				env_choice = new EnvChoicePoint(env_trace, env_trace->current_node());
				 // makes env_trace to take a step and update its current node
				if(!env_choice->ChooseNext()) {
					// no more steps, this means that the environment has ended or no next point
					// thus we exit env thread now
					break;
				}

			}

			env_choice->SetAndConsumeCurrent();

		} // end if

		// force yield after each step
		// TODO(elmas): use the last accesses of env instead of NULL
		FORCE_YIELD("ENV", NULL);

	} // end while
	return NULL;
}

/********************************************************************************/

// this is a API call, given a set of threads, makes a modular check
void ThreadModularScenario::TestCase() {
	TEST_FORALL(); // we check all possible executions

	// add environment thread to the group
	Coroutine* env_co = CREATE_THREAD("ENV", env_thread_function, &env_trace_);

	// add a choice point for choosing which member to run (or reuse it if already exists)
	MemberChoicePoint* member_choice = NULL;
	ChoicePoint* choice_point = ChoicePoint::GetCurrent();
	if(choice_point != NULL) {
		VLOG(2) << SC_TITLE << "Reusing member choice point";

		member_choice = ASINSTANCEOF(choice_point, MemberChoicePoint*);
	} else {
		VLOG(2) << SC_TITLE << "Creating new member choice point";

		CoroutinePtrSet members = group_.GetMemberSet();
		// remove env thread
		members.erase(members.find(env_co));
		member_choice = new MemberChoicePoint(members);
		// no need to call ChooseNext, as member_choice.itr_ is already pointing to the first member
	}

	Coroutine* co = member_choice->GetNext();
	if(co == NULL) {
		VLOG(2) << SC_TITLE << "All members were used, so backtracking";
		// no more members, so backtrack (and end the execution)
		TRIGGER_BACKTRACK();
	}
	safe_assert(co != env_co);

	member_choice->SetAndConsumeCurrent();

	// run one member at a time concurrently with env_thread
	VLOG(2) << SC_TITLE << "Modular check with coroutine " << co->name();
	{ WITH(co, env_co);
		UNTIL_ALL_END {
			UNTIL_STAR()->TRANSFER_STAR();
		}
	}
}

/********************************************************************************/

// this is called by EnvChoicePoint to find out the next following target of current_node
// next_node is the last return value
EnvNodePtr EnvTrace::Step(EnvNodePtr current_node, EnvNodePtr next_node /*= EnvNodePtr()*/) {
	// decide whether to stay in the same node or transit to another node
	// TODO(elmas)...
	current_node_ = EnvNodePtr();
	return current_node_;
}

/********************************************************************************/

// make a transition from current node to a matching node
// if there is no matching node, then we create a new node (but not add it to env_graph)
void EnvTrace::OnAccess(Coroutine* current, SharedAccess* access) {
	safe_assert(!nodes_.empty());

	EnvNodePtr node;
	if(access->is_read()) {
		// on a read access we use the existing current node
		node = nodes_.back();
	} else {
		// on a write access we create a clone of the existing node
		node = nodes_.empty() ? env_graph_->MakeNewNode() : env_graph_->MakeNewNode(nodes_.back());
		nodes_.push_back(node);
	}

	// update the node with the information about the access
	node->Update(current, access->cell());

	// update the current node of the trace
	current_node_ = node;
}

/********************************************************************************/

void EnvTrace::Restart(EnvGraph* g /*=NULL*/) {
	// delete all states
	delete_nodes();
	if(env_graph_ == NULL || g != NULL) {
		env_graph_ = g;
	}
	safe_assert(env_graph_ != NULL);
	current_node_ = env_graph_->start_node();
	nodes_.push_back(current_node_);
}

/********************************************************************************/

void EnvTrace::delete_nodes() {
	// nodes are shared pointers, so do not delete them, just clear the list
	// shared_ptr takes care of the rest
	nodes_.clear();
}

/********************************************************************************/

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

/********************************************************************************/

void EnvNode::AddEdge(EnvNodePtr node) {
	edges_.insert(node);
}

/********************************************************************************/

bool EnvNode::operator ==(const EnvNode& node) {
	// compare globals
	// TODO(elmas): improve this comparison, as states does not have to match exactly
	return globals_ == node.globals_;
}

/********************************************************************************/

EnvNodePtr EnvNode::Clone(bool clone_ginfo /*= false*/) {
	EnvNodePtr node = EnvNodePtr(new EnvNode());
	node->globals_.Update(&globals_);

	// clone graph information is requested (default: false)
	if(clone_ginfo) {
		node->edges_ = edges_;
		node->env_graph_ = env_graph_;
	}
	return node;
}

/********************************************************************************/

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

/********************************************************************************/

void MemoryMap::Update(MemoryCellBase* cell) {
	ADDRINT mem = cell->int_address();
	CellMap::iterator itr = memToCell_.find(mem);
	if(itr != memToCell_.end()) {
		itr->second->update_value(cell);
	} else {
		memToCell_[mem] = cell->Clone();
	}
}

/********************************************************************************/

std::string MemoryMap::ToString() {
	std::stringstream s;

	for(CellMap::iterator itr = memToCell_.begin(); itr != memToCell_.end(); ++itr) {
		s << itr->second->ToString() << " ";
	}

	return s.str();
}

/********************************************************************************/

void MemoryMap::delete_cells() {
	for(CellMap::iterator itr = memToCell_.begin(); itr != memToCell_.end(); ++itr) {
		MemoryCellBase* cell = itr->second;
		delete cell;
		memToCell_.erase(itr);
	}
	safe_assert(memToCell_.empty());
}

/********************************************************************************/

bool MemoryMap::operator ==(const MemoryMap& other) {
	return memToCell_ == other.memToCell_;
}

/********************************************************************************/

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
	if(existing != NULL) {
		return existing->Clone(/*clone_edges=*/false); // do not clone edges
	} else {
		return EnvNodePtr(new EnvNode());
	}
}

/********************************************************************************/

void EnvGraph::AddNode(EnvNodePtr node) {
	// we have at least start node
	safe_assert(start_node_ != NULL);
	nodes_.insert(node);
}

/********************************************************************************/

EnvNodePtr EnvGraph::SearchForEquivNode(EnvNodePtr node) {
	safe_assert(node != NULL);

	if(nodes_.find(node) != nodes_.end()) {
		// node already exists in the graph
		return node;
	}

	// check every node in the graph to see if there is any equiv. one
	for(EnvNodePtrSet::iterator itr = nodes_.begin(); itr != nodes_.end(); ++itr) {
		EnvNodePtr n = (*itr);
		if((*n) == (*node)) { // uses overloaded operator ==
			return n;
		}
	}

	// could not find an equivalent node
	return EnvNodePtr();
}

/********************************************************************************/

void EnvGraph::Update(EnvTrace* trace) {
	EnvNodePtr prev;
	// traverse the trace, and add new nodes and connections between nodes
	EnvNodePtrList* nodes = trace->nodes();
	for(EnvNodePtrList::iterator itr = nodes->begin(); itr < nodes->end(); ++itr) {
		EnvNodePtr node = (*itr);

		// check if an equivalent node exists in the graph
		if(SearchForEquivNode(node) == NULL) {
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

/********************************************************************************/

void EnvGraph::delete_nodes() {
	// nodes are shared pointers, so do not delete them, just clear the list
	// shared_ptr takes care of the rest
	nodes_.clear();
}

/********************************************************************************/

} // end namespace

