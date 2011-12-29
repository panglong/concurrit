
#include "concurrit.h"
#include "math.h"
#include <assert.h>

#include "increment.cpp"

void* increment_thread(void* p) {
	Counter* counter = static_cast<Counter*>(p);

	counter->increment();

	return NULL;
}

class CounterScenario : public Scenario {
public:

	CounterScenario(const char* name) : Scenario(name) {}

	virtual void SetUp() {
		counter = new Counter();
	}

	virtual void TearDown() {
		delete counter;
	}

protected:
	Counter* counter;

};

#define COUNTER (this->counter)

/************************************************************************/

class Scenario0 : public CounterScenario {
public:
	Scenario0() : CounterScenario("SearchTest0") {}

	void TestCase() {

		coroutine_t t1 = CreateThread("t1", increment_thread, COUNTER);
		coroutine_t t2 = CreateThread("t2", increment_thread, COUNTER);
		coroutine_t t3 = CreateThread("t3", increment_thread, COUNTER);

		{
//			EXHAUSTIVE_SEARCH();
			CONTEXT_BOUNDED_EXHAUSTIVE_SEARCH(3);
//			NDSEQ_SEARCH();
		}

		ASSERT(COUNTER->get() == 3);

		printf("Counter: %d\n", COUNTER->get());
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
