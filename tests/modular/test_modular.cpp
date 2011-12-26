
#include "counit.h"
#include "math.h"
#include <assert.h>

using namespace counit;

struct Counter {
	int x;
	int l;

	Counter() {
		x = 0;
		l = 0;
	}

	void increment() {

		while(true) {
			int t = YIELD_READ("L1", x);
			int k = t + 1;

			//lock();
			if(YIELD_READ("L2", x) == t) {
				YIELD_WRITE("L3", x) = k;
				printf("Updated x\n");
//				unlock();
				break;
			}
			//unlock();
		}

	}

	void lock() {
		ASSUME(YIELD_READ("lock1", l) == 0);
		WRITE_YIELD("lock2", l) = 1;
	}

	void unlock() {
		WRITE_YIELD("unlock", l) = 0;
	}

	int get() {
		return x;
	}
};


void* increment_thread(void* p) {
	Counter* counter = static_cast<Counter*>(p);

	counter->increment();

	return NULL;
}

class CounterScenario : public ThreadModularScenario {
public:

	CounterScenario(const char* name) : ThreadModularScenario(name) {}
	~CounterScenario() {}

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

class Scenario1 : public CounterScenario {
public:
	Scenario1() : CounterScenario("SearchTest1") {}

	void SetUp() {
		CounterScenario::SetUp();

		coroutine_t t1 = CreateThread("t1", increment_thread, COUNTER);
		coroutine_t t2 = CreateThread("t2", increment_thread, COUNTER);
//		coroutine_t t3 = CreateThread("t3", increment_thread, COUNTER);
//		coroutine_t t4 = CreateThread("t4", increment_thread, COUNTER);

		printf("Counter: %d\n", COUNTER->get());
	}
};


/************************************************************************/

int main(int argv, char** argc) {

	BeginCounit();

	Suite suite;

//	suite.AddScenario(new Scenario0());
	suite.AddScenario(new Scenario1());
	suite.RunAll();

	EndCounit();

	return 0;
}
