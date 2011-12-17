

#include "counit.h"
#include <stdlib.h>

using namespace counit;


#define NO_VALUE 0

class Node {
public:
	Node(int data) {
		data_ = data;
		next_ = NULL;
	}

	int data_;
	Node* next_;
};

class LinkedList {

public:

	LinkedList() {
		// make head and tail point to a dummy node
		head_ = tail_ = new Node(NO_VALUE); // sets next to NULL
	}


	void AddLast(int data) {
		Node* node = new Node(data);

		// update tail's next pointer
		tail_->next_ = node;

		//YIELD("L1@AddLast", &tail_);

		// update tail
		tail_ = node;
	}

	int RemoveFirst() {
		if(head_->next_ == NULL) {
			return NO_VALUE;
		}

		//YIELD("L2@RemoveFirst", &head_->next_);

		// get first node (after dummy)
		Node* node = head_->next_;

		// update the first element
		Node* new_first = node->next_;
		head_->next_ = new_first;

		// check if the list became empty
		if(new_first == NULL) {
			tail_ = head_;
		}
	}

	int GetFirst() {
		if(IsEmpty()) return NO_VALUE;
		return head_->next_->data_;
	}

	int GetLast() {
		if(IsEmpty()) return NO_VALUE;
		return tail_->data_;
	}

	bool Contains(int query) {
		Node* node = head_->next_;
		while(node != NULL) {
			if(node->data_ == query) {
				return true;
			}
		}
		return false;
	}

	bool CheckInvariant() {
		return head_ != NULL && tail_ != NULL &&
			  (head_->next_ != NULL || tail_ == head_);
	}

	bool IsEmpty() {
		// any node between head and tail?
		return head_->next_ == NULL;
	}

	~LinkedList() {
		while(head_ != NULL) {
			Node* tmp = head_;
			head_ = head_->next_;
			delete tmp;
		}
	}

	Node* head_;
	Node* tail_;

};

/************************************************************************/

void* Add_1_Thread(void* arg) {
	LinkedList* list = static_cast<LinkedList*>(arg);

	list->AddLast(1);

	return NULL; //ignore this
}

void* Add_2_Thread(void* arg) {
	LinkedList* list = static_cast<LinkedList*>(arg);

	list->AddLast(2);

	return NULL; //ignore this
}

void* RemoveThread(void* arg) {
	LinkedList* list = static_cast<LinkedList*>(arg);

	int data = list->RemoveFirst();

	return NULL; //ignore this
}

/************************************************************************/

class ListScenario : public Scenario {
public:

	ListScenario(const char* name) : Scenario(name) {}

	LinkedList* list;

	virtual void SetUp() {
		list = new LinkedList();

//		for(int i = 0; i < 10; i++) {
//			list->AddLast(rand());
//		}
	}

	virtual void TearDown() {
		if(list != NULL) {
			delete list;
		}
	}

};

/************************************************************************/

class TestScenario1 : public ListScenario {
public:

	TestScenario1() : ListScenario("TestScenario1") {}

	void TestCase() {
		coroutine_t t_add = CreateThread("add_1", Add_1_Thread, list);
		coroutine_t t_remove = CreateThread("remove", RemoveThread, list);

		UntilEnd()->Transfer(t_add);

		Assert(list->Contains(1));

		UntilEnd()->Transfer(t_remove);

		Assert(list->IsEmpty());
		Assert(list->CheckInvariant());
	}

};

/************************************************************************/

class TestScenario2 : public ListScenario {
public:

	TestScenario2() : ListScenario("TestScenario2") {}

	void TestCase() {
		coroutine_t t_add = CreateThread("add_1", Add_1_Thread, list);
		coroutine_t t_remove = CreateThread("remove", RemoveThread, list);

		UntilEnd()->TransferStar();

		UntilEnd()->TransferStar();

		Assume(!list->IsEmpty());
		Assert(list->CheckInvariant());
	}

};

/************************************************************************/

class TestScenario3 : public ListScenario {
public:

	TestScenario3() : ListScenario("TestScenario3") {}

	void TestCase() {
		CheckForall();

		coroutine_t t_add = CreateThread("add_1", Add_1_Thread, list);
		coroutine_t t_remove = CreateThread("remove", RemoveThread, list);

		UntilEnd()->TransferStar();

		UntilEnd()->TransferStar();

		Assume(!list->IsEmpty());
		Assert(list->CheckInvariant());
	}

};


/************************************************************************/

class TestScenario4 : public ListScenario {
public:

	TestScenario4() : ListScenario("TestScenario4") {}

	void TestCase() {
		CheckForall();

		coroutine_t t_add = CreateThread("add_1", Add_1_Thread, list);
		coroutine_t t_remove = CreateThread("remove", RemoveThread, list);

		UntilStar()->TransferStar();

		UntilStar()->TransferStar();

		Assume(!list->IsEmpty());
		Assume(list->CheckInvariant());
	}

};


/************************************************************************/

class TestScenario5 : public ListScenario {
public:

	TestScenario5() : ListScenario("TestScenario5") {}

	void TestCase() {
		coroutine_t t_add = CreateThread("add_1", Add_1_Thread, list);
		coroutine_t t_remove = CreateThread("remove", RemoveThread, list);

		YieldPoint* point = UntilStar()->Transfer(t_add)->AsYield();
		Assume(point->label() != ENDING_LABEL);

		UntilEnd()->Transfer(t_remove);

		UntilEnd()->Transfer(t_add);

		Assume(list->IsEmpty());
		Assert(list->CheckInvariant());
	}

};

/************************************************************************/

int main(int argv, char** argc) {

	BeginCounit();

	Suite suite;
	suite.AddScenario(new TestScenario1());
	suite.AddScenario(new TestScenario2());
	suite.AddScenario(new TestScenario3());
	suite.AddScenario(new TestScenario4());
	suite.AddScenario(new TestScenario5());

	suite.RunAll();

	EndCounit();

	return 0;
}
