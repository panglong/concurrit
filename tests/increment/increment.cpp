
#include "concurrit.h"

using namespace counit;

class Counter {

public:

	explicit Counter(int i) { x_ = i; }

	void increment();
	int get();

private:
	int x_;

};

void Counter::increment() {
	//YIELD("L1", READ(&x_));
	int t = YIELD_READ("L1", x_);

	t = t + 1;

	//YIELD("L2", WRITE(&x_));
	YIELD_WRITE("L2", x_) = t;

}

int Counter::get() {
	return x_;
}

/************************************************************************/

void* increment(void* arg) {
	Counter* counter = static_cast<Counter*>(arg);
	counter->increment();
	return NULL;
}

/************************************************************************/


class IncrementScenario1 : public Scenario {
public:

	IncrementScenario1() : Scenario("IncrementTest1") {}

	void TestCase() {
		Counter counter = Counter(0);

		coroutine_t t1 = CreateThread("increment1", increment, &counter);
		coroutine_t t2 = CreateThread("increment2", increment, &counter);

		UNTIL_STAR()->TRANSFER(t1);
		UNTIL_STAR()->TRANSFER(t2);

		UNTIL_END()->TRANSFER(t1);
		UNTIL_END()->TRANSFER(t2);

		printf("Counter: %d\n", counter.get());
		Assert(counter.get() == 2);
	}

};

/************************************************************************/

class IncrementScenario2 : public Scenario {
public:

	IncrementScenario2() : Scenario("IncrementTest2") {}

	void TestCase() {
		Counter counter = Counter(0);

		coroutine_t t1 = CreateThread("increment1", increment, &counter);
		coroutine_t t2 = CreateThread("increment2", increment, &counter);

		NDSEQ_SEARCH();

		printf("Counter: %d\n", counter.get());
//		Assert(counter.get() == 1);
	}

};

/************************************************************************/

int main(int argv, char** argc) {

	BeginCounit();

	Suite suite;

	suite.AddScenario(new IncrementScenario1());
	suite.AddScenario(new IncrementScenario2());

	suite.RunAll();


	IncrementScenario2 scenario;

	Result* result = scenario.Explore();
	printf("Test result: %s\n", result->ToString().c_str());

	if(INSTANCEOF(result, ExistsResult*)) {
		ExistsResult* eresult = ASINSTANCEOF(result, ExistsResult*);
		eresult->schedule()->SaveToFile(InWorkDir("scenario1.sch"));

		IncrementScenario1 scenario2;

		scenario2.LoadScheduleFromFile(InWorkDir("scenario1.sch"));

		Result* result2 = scenario2.Explore();
		printf("Test result: %s\n", result->ToString().c_str());
	}

	EndCounit();

	return 0;
}
