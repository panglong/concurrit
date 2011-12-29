
#include "concurrit.h"
#include "math.h"
#include <assert.h>

#include "stack.cpp"

void* stack_thread(void* p) {
	Stack* stack = static_cast<Stack*>(p);

	stack->push(1);

	stack->push(2);

	return NULL;
}

class StackScenario : public Scenario {
public:

	StackScenario(const char* name) : Scenario(name) {}

	virtual void SetUp() {
		stack = new Stack();
	}

	virtual void TearDown() {
		delete stack;
	}

protected:
	Stack* stack;

};

#define STACK (this->stack)

/************************************************************************/

class Scenario0 : public StackScenario {
public:
	Scenario0() : StackScenario("SearchTest0") {}

	void TestCase() {

		coroutine_t t1 = CreateThread("t1", stack_thread, STACK);
		coroutine_t t2 = CreateThread("t2", stack_thread, STACK);
		coroutine_t t3 = CreateThread("t3", stack_thread, STACK);

		{ //WITHOUT(t3);
			CONTEXT_BOUNDED_EXHAUSTIVE_SEARCH(3);
		}

		ASSERT(STACK->size() == 4);

		printf("Stack size: %d\n", STACK->size());
	}
};

/************************************************************************/

/************************************************************************/

int main(int argv, char** argc) {

	BeginCounit();

	Suite suite;

	suite.AddScenario(new Scenario0());
	suite.RunAll();

	EndCounit();

	return 0;
}
