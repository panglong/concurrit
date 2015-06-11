# CONCURRIT: A Domain Specific Language for Testing Concurrent Programs #

**CONCURRIT** is a novel testing technique and domain speciﬁc language (DSL) for unit and system testing of concurrent
programs. Using CONCURRIT, a programmer can control and guide
the thread schedule of the software-under-test (SUT) in both a formal and concise way to examine only the intended interleavings
of threads.

Our DSL provides constructs to express nondeterministic choice, with which one can specify a set of thread schedules to
be systematically enumerated similarly to model checking. In contrast with the traditional notion of model checking, in our approach, the programmer can write a CONCURRIT
test that describes the controlled-nondeterminism for part of the
thread schedule, leaving the rest of the schedule unspeciﬁed. We
call the result **tolerant model checking**, where a search of thread
schedules in the presence of uncontrolled nondeterminism can still
be systematic and exhaustive with respect to the CONCURRIT test.
We have applied CONCURRIT in both unit and system testing
of a collection of benchmarks including the Mozilla’s SpiderMonkey JavaScript engine, Memcached, the Apache HTTP server, and
MySQL.

_**NEWS**_: See our position paper in [HOTPAR 2012](https://www.usenix.org/conference/hotpar12).

_**NEWS**_: See our technical report about [CONCURRIT](https://sites.google.com/site/tayfunelmas/concurrit-techreport.pdf).

### Documentation ###
See our wiki pages for more information about how to install and use CONCURRIT.