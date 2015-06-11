# Test file #

The generic contents of a test file is shown below.
The file contains two tests, named TestName1 and TestName2.
```
#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

/**********************************************************/
CONCURRIT_BEGIN_TEST(TestName1, "Test1 description")

TESTCASE() { 
... // Test1 in CONCURRIT.
}

CONCURRIT_END_TEST(TestName1)
/**********************************************************/

/**********************************************************/
CONCURRIT_BEGIN_TEST(TestName2, "Test2 description")

TESTCASE() { 
... // Test2 in CONCURRIT.
}
/**********************************************************/

CONCURRIT_END_TEST(TestName2)


CONCURRIT_END_MAIN()
```

Each test file include the header file `concurrit.h` and all the tests are written between `CONCURRIT_BEGIN_MAIN()` and `CONCURRIT_END_MAIN()`.

Each test is written between `CONCURRIT_BEGIN_TEST(TestName, "Test description")` and `CONCURRIT_END_TEST(TestName1)`. The test code is written as a function `TESTCASE() { ... } `.

Since CONCURRIT DSL is embedded in C++, the following are allowed: