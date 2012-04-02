#include <stdio.h>

#include "concurrit.h"
using namespace concurrit;

#include "increment.h"

#define NUM_THREADS 2

void* increment_routine(void* arg)
{

  NBCounter * counter = (NBCounter*) arg;

  safe_assert(counter != NULL);
  counter->increment();

  return NULL;
}

class INCScenario : public Scenario {
public:

	INCScenario() : Scenario("NB-Increment scenario") {
		counter = NULL;
	}
	~INCScenario() {

	}

	void SetUp() {
		counter = new NBCounter();
	}

	void TearDown() {
		if(counter != NULL)
			delete counter;
		counter = NULL;
	}

	NBCounter* counter;
	coroutine_t threads[NUM_THREADS];

	void TestCase() {

		TEST_FORALL();

		for (int i = 0; i < NUM_THREADS; i++)
		{
			threads[i] = CREATE_THREAD(i, increment_routine, (void*)counter, true);
		}


		while(STAR)
		{
//				EXISTS(t);
//				DSLTransition(TransitionPredicate::True(), t);

//			DSLTransition(TransitionPredicate::True());

			EXISTS(t);
			DSLTransferUntil(t, TransitionPredicate::True());
		}
	}

};


int main(int argc, char ** argv)
{
	INIT_CONCURRIT(argc, argv);

	Suite suite;

	suite.AddScenario(new INCScenario());

	suite.RunAll();

  return 0;
}

