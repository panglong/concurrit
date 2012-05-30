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

void ThreadModularScenario::Start() {
	super::Start();

	// restart memory trace
	env_trace_.Restart();
}

/********************************************************************************/

ConcurritException* ThreadModularScenario::RunOnce() throw() {
	ConcurritException* exc = super::RunOnce();

	// print the memory trace:
	printf("Memory trace:\n%s\n", env_trace_.ToString().c_str());

	printf("EnvGraph has %ld nodes\n", env_graph_.nodes()->size());
	printf("%s\n", env_graph_.ToString().c_str());

	// add memory trace to env graph
	env_graph_.Update(&env_trace_);

	return exc;
}

/********************************************************************************/

void* env_thread_function(void* p) {
	CHECK(p != NULL);
	EnvTrace* env_trace = static_cast<EnvTrace*>(p);
	EnvNodePtr current_node = env_trace->current_node();
	for(;true;) {
		MYLOG(2) << "Running env thread context";

		Coroutine* current = Coroutine::Current();
		safe_assert(!current->IsMain());

//		SchedulePoint* main_point = current->yield_point();
//		if(main_point != NULL) {
//			main_point = main_point->AsYield()->prev();
//
//			safe_assert(main_point != NULL && main_point->IsTransfer());
//			safe_assert(main_point->AsYield()->source()->IsMain());

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

			MYLOG(2) << "env_trace is taking a step";

			// add a choice point for choosing which member to run (or reuse it if already exists)
			EnvChoicePoint* env_choice = NULL;
			ChoicePoint* choice_point = ChoicePoint::GetCurrent();
			if(choice_point != NULL) {
				MYLOG(2) << "Reusing env choice point";

				env_choice = ASINSTANCEOF(choice_point, EnvChoicePoint*);
				EnvNodePtr root_node = env_choice->root_node();
				EnvNodePtr next_node = env_choice->GetNext();

				safe_assert(next_node != NULL);
				// root note must be in the env graph
				safe_assert(env_trace->env_graph()->ContainsNode(root_node));

				// we backtrack if root_node is not the same as current_node and env_thread and env_trace
				if(current_node != root_node || env_trace->current_node() != root_node) {
					TRIGGER_BACKTRACK();
				}

				// set the next current node of the trace
				env_trace->SetNext(next_node);
			} else {
				MYLOG(2) << "Creating new env choice point";

				EnvNodePtr root_node = env_trace->current_node();

				// if the current node of env_trace is not the same as the current node of env,
				// then, this means the local thread lead the program out of env graph
				if(root_node != current_node) {
					break;
				}

				// first taking of this transition
				// gets the current node of env_trace as the current node
				env_choice = new EnvChoicePoint(env_trace, root_node);
				 // makes env_trace to take a step and update its current node
				if(!env_choice->ChooseNext()) {
					// no more steps, this means that the environment has ended or no next point
					// thus we exit env thread now
					break;
				}
			}

			safe_assert(env_choice != NULL);
			env_choice->SetAndConsumeCurrent();

			// update current node of env thread
			current_node = env_trace->current_node();
			safe_assert(env_trace->env_graph()->ContainsNode(current_node));

//		} // end if

		// force yield after each step
		// TODO(elmas): use the last accesses of env instead of NULL
		MYLOG(2) << "Yielding from env";
		FORCE_YIELD("ENV", NULL);

	} // end while
	return NULL;
}

/********************************************************************************/

// this is a API call, given a set of threads, makes a modular check
void ThreadModularScenario::TestCase() {

//	TEST_FORALL(); // we check all possible executions
//	DISABLE_DPOR(); // disable dynamic partial order reduction
//
//	// add environment thread to the group
//	ThreadVarPtr env_var = CREATE_THREAD(env_thread_function, &env_trace_);
//	safe_assert(env_var != NULL && !env_var->is_empty());
//	Coroutine* env_co = env_var->thread();
//
//	// add a choice point for choosing which member to run (or reuse it if already exists)
//	MemberChoicePoint* member_choice = NULL;
//	ChoicePoint* choice_point = ChoicePoint::GetCurrent();
//	if(choice_point != NULL) {
//		MYLOG(2) << SC_TITLE << "Reusing member choice point";
//
//		member_choice = ASINSTANCEOF(choice_point, MemberChoicePoint*);
//	} else {
//		MYLOG(2) << SC_TITLE << "Creating new member choice point";
//
//		CoroutinePtrSet members;
//		group_.GetMemberSet(&members);
//		// remove env thread
//		members.erase(members.find(env_co));
//		member_choice = new MemberChoicePoint(members);
//		// no need to call ChooseNext, as member_choice.itr_ is already pointing to the first member
//	}
//
//	Coroutine* co = member_choice->GetNext();
//	if(co == NULL) {
//		MYLOG(2) << SC_TITLE << "All members were used, so backtracking";
//		// no more members, so backtrack (and end the execution)
//		TRIGGER_BACKTRACK();
//	}
//	safe_assert(co != env_co);
//
//	member_choice->SetAndConsumeCurrent();
//
//	// run one member at a time concurrently with env_thread
//	MYLOG(2) << SC_TITLE << "Modular check with coroutine " << co->tid();
//
//	{ WITH(env_co, co);
//		UNTIL_ALL_END {
//			UNTIL_STAR()->TRANSFER_STAR();
//		}
//		if(env_graph_.size() >= 10) {
//			TRIGGER_TERMINATE_SEARCH();
//		}
//	}
}

/********************************************************************************/

void EnvTrace::SetNext(EnvNodePtr next_node) {
	safe_assert(next_node != NULL);
	safe_assert(!nodes_.empty() || current_node_ == NULL);
	safe_assert(nodes_.empty() || nodes_.back() == current_node_);
	if(current_node_ == NULL || (current_node_ != next_node && !current_node_->IsEquiv(*next_node))) {
		current_node_ = next_node;
		nodes_.push_back(current_node_);

		// check and update the current program state with the chosen env node
		if(!current_node_->CheckAndUpdateProgramState()) {
			TRIGGER_BACKTRACK();
		}
	}
}

/********************************************************************************/

EnvNodePtr EnvTrace::Step(EnvChoicePoint* point) {
	MYLOG(2) << "Next step in env trace.";
	safe_assert(point != NULL);

	EnvNodePtr next_node = ChooseNext(point);
	safe_assert(next_node != NULL || point->GetNext() == NULL);

	MYLOG(0) << "Root node is: " << point->root_node()->ToString(false);
	MYLOG(0) << "Current node is: " << (next_node == NULL ? "NULL" : next_node->ToString(false));

	if(next_node != NULL) {
		// update the currently-tracked state
		SetNext(next_node);
	}

	return next_node;
}

/********************************************************************************/

// this is called by EnvChoicePoint to find out the next following target of current_node
EnvNodePtr EnvTrace::ChooseNext(EnvChoicePoint* point) {
	MYLOG(2) << "Choosing next target for env choice point.";

//	EnvNodePtrSet* gray_nodes = point->gray_nodes();
	EnvNodePtrSet* black_nodes = point->black_nodes();

	EnvNodePtrIteratorList* path = point->path();
	if(point->GetNext() == NULL) {
		return EnvNodePtr(); // = NULL
	}
	safe_assert(!path->empty());

	EnvNodePtr next_node; // = NULL
	EnvNodePtrIterator iter = path->back(); path->pop_back();

	//------------------------------------
	// forward DFS (find the next leaf)
	while(iter.has_current()) {
		next_node = iter.current();
		safe_assert(next_node != NULL);

//		if(gray_nodes->find(next_node) != gray_nodes->end()) {
//			TRIGGER_INTERNAL_EXCEPTION("Cycle found in the env graph!");
//		}

		if(black_nodes->find(next_node) != black_nodes->end()) {
			iter.next();
		} else {
			// mark current_node as GRAY
//			gray_nodes->insert(next_node);
			black_nodes->insert(next_node);
			path->push_back(iter);
			iter = EnvNodePtrIterator(next_node->out_edges());
		}
	}
	if(next_node == NULL) {
		// backward DFS (visit non-leaves)
		if(path->empty()) {
			return EnvNodePtr(); // NULL
		}
		iter = path->back(); path->pop_back();
		safe_assert(iter.has_current());
		next_node = iter.current();
		safe_assert(next_node != NULL);
		iter.next();
		path->push_back(iter);
	}
	//------------------------------------

	safe_assert(next_node != NULL);

	if(next_node == point->root_node()) {
		if(path->size() > 1) {
			TRIGGER_INTERNAL_EXCEPTION("Cycle found in the env graph!!!");
		} else {
			// TODO(elmas): if current_node is root_node and we cannot stutter, return NULL
			// this happens when this choice point is invoked the last time
			return EnvNodePtr(); // = NULL
		}
	}

	// mark next_node as BLACK
//	gray_nodes->erase(next_node);
	black_nodes->insert(next_node);

	return next_node;
}

/********************************************************************************/

// make a transition from current node to a matching node
// if there is no matching node, then we create a new node (but not add it to env_graph)
void EnvTrace::OnAccess(Coroutine* current, SharedAccess* access) {
	safe_assert(!nodes_.empty() && nodes_.back() == current_node_);

	EnvNodePtr node = (access->is_read()) ? (nodes_.back()) : (env_graph_->MakeNewNode(nodes_.back()));

	// update the node with the information about the access
	node->Update(current, access->cell());

	EnvNodePtr next_node = env_graph_->SearchForEquivNode(node);
	if(next_node == NULL) {
		next_node = node;
	}

	// update the currently-tracked state
	SetNext(next_node);
}

/********************************************************************************/

void EnvTrace::Restart(EnvGraph* g /*=NULL*/) {
	// delete all states
	delete_nodes();
	if(env_graph_ == NULL || g != NULL) {
		env_graph_ = g;
	}
	safe_assert(env_graph_ != NULL);

	// update the currently-tracked state
	current_node_ = EnvNodePtr(); // = NULL
	SetNext(env_graph_->start_node());
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

	for(EnvNodePtrList::iterator itr = nodes_.begin(); itr < nodes_.end(); ++itr) {
		EnvNodePtr node = (*itr);
		s << node->ToString() << "\n";
		s << "********************\n";
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
	out_edges_.insert(node);
}

/********************************************************************************/

bool EnvNode::IsEquiv(const EnvNode& node) {
	if(&node == this) return true;
	// compare globals
	// TODO(elmas): improve this comparison, as states does not have to match exactly
	return globals_.IsEquiv(node.globals_);
}

/********************************************************************************/

EnvNodePtr EnvNode::Clone(bool clone_ginfo /*= false*/) {
	EnvNodePtr node = EnvNodePtr(new EnvNode());
	node->globals_.Update(&globals_);

	// clone graph information is requested (default: false)
	if(clone_ginfo) {
		node->id_ = id_;
		node->out_edges_ = out_edges_;
		node->env_graph_ = env_graph_;
	}
	return node;
}

/********************************************************************************/

std::string EnvNode::ToString(bool print_memory /*= true*/) {
	std::stringstream s;

	s <<"Id: " << id_ ;
	s << " Out edges: ";
	for(EnvNodePtrSet::iterator itr = out_edges_.begin(); itr != out_edges_.end(); ++itr) {
		EnvNodePtr node = (*itr);
		s << node->id() << " ";
	}
	if(print_memory) {
		s << " MemoryMap: " << globals_.ToString();
	}

	return s.str();
}

/********************************************************************************/

bool EnvNode::CheckAndUpdateProgramState() {
	globals_.UpdateProgramState();
	return true; // TODO(elmas)
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

void MemoryMap::UpdateProgramState() {
	for(CellMap::iterator itr = memToCell_.begin(); itr != memToCell_.end(); ++itr) {
		MemoryCellBase* cell = itr->second;
		cell->update_memory();
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

bool MemoryMap::IsEquiv(const MemoryMap& other) {
	if(memToCell_.size() != other.memToCell_.size()) {
		return false;
	}

	for(CellMap::iterator itr = memToCell_.begin(); itr != memToCell_.end(); ++itr) {
		MemoryCellBase* cell = itr->second;
		CellMap::const_iterator itr2 = other.memToCell_.find(cell->int_address());
		if(itr2 == other.memToCell_.end() || !cell->IsEquiv(itr2->second)) {
			return false;
		}
	}

	return true;
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
	node->set_id(next_node_id_);
	next_node_id_++;
}

/********************************************************************************/

bool EnvGraph::ContainsNode(EnvNodePtr node) {
	return (nodes_.find(node) != nodes_.end());
}

/********************************************************************************/

EnvNodePtr EnvGraph::SearchForEquivNode(EnvNodePtr node) {
	safe_assert(node != NULL);

	if(ContainsNode(node)) {
		// node already exists in the graph
		return node;
	}

	// check every node in the graph to see if there is any equiv. one
	for(EnvNodePtrSet::iterator itr = nodes_.begin(); itr != nodes_.end(); ++itr) {
		EnvNodePtr n = (*itr);
		if(n->IsEquiv(*node)) {
			return n;
		}
	}

	// could not find an equivalent node
	return EnvNodePtr(); // = NULL
}

/********************************************************************************/

void EnvGraph::Update(EnvTrace* trace) {
	EnvNodePtr prev; // = NULL
	// traverse the trace, and add new nodes and connections between nodes
	EnvNodePtrList* nodes = trace->nodes();
	for(EnvNodePtrList::iterator itr = nodes->begin(); itr < nodes->end(); ++itr) {
		EnvNodePtr node = (*itr);

		// check if an equivalent node exists in the graph
		EnvNodePtr equiv_node = SearchForEquivNode(node);
		if(equiv_node == NULL) {
			// add the node
			AddNode(node);
			equiv_node = node;
		}
		safe_assert(equiv_node != NULL);
		safe_assert(prev != equiv_node);
		if(prev != NULL) {
			// add edge from prev to node
			prev->AddEdge(equiv_node);
		}
		prev = equiv_node;
	}
}

/********************************************************************************/

void EnvGraph::delete_nodes() {
	// nodes are shared pointers, so do not delete them, just clear the list
	// shared_ptr takes care of the rest
	nodes_.clear();
}

/********************************************************************************/

std::string EnvGraph::ToString() {
	std::stringstream s;

	s << "***** Begin Nodes:\n";
	for(EnvNodePtrSet::iterator itr = nodes_.begin(); itr != nodes_.end(); ++itr) {
		EnvNodePtr node = (*itr);
		s << node->ToString() << "\n";
	}
	s << "***** End Nodes:\n";

	return s.str();
}

/********************************************************************************/


} // end namespace

