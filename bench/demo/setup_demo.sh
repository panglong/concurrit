# This file sets up the environment for the Concurrit demo.

alias setup_demo='source $CONCURRIT_HOME/bench/demo/setup_demo.sh && compile_demo && uninstrument_demo'

alias compile_demo='$CONCURRIT_HOME/scripts/compile_bench.sh demo'

# Run SUT uncontrolled.
alias run_demo='$CONCURRIT_HOME/scripts/run_bench.sh demo -u'

# Run SUT controlled by the test.
alias run_test='$CONCURRIT_HOME/scripts/run_bench.sh demo -c -s -t'

alias show_trace='emacs $CONCURRIT_HOME/work/trace.txt'
alias show_demo='emacs $CONCURRIT_HOME/bench/demo/src/boundedBuffer.c'
alias show_test='emacs $CONCURRIT_HOME/bench/demo/src/boundedBufferTest.cpp'

alias add_sleep_demo='cp -f $CONCURRIT_HOME/bench/demo/src/boundedBuffer_Sleep.c $CONCURRIT_HOME/bench/demo/src/boundedBuffer.c && compile_demo && show_demo'
alias instrument_demo='cp -f $CONCURRIT_HOME/bench/demo/src/boundedBuffer_Instrumented.c $CONCURRIT_HOME/bench/demo/src/boundedBuffer.c && compile_demo && show_demo'
alias uninstrument_demo='cp -f $CONCURRIT_HOME/bench/demo/src/boundedBuffer_Original.c $CONCURRIT_HOME/bench/demo/src/boundedBuffer.c && compile_demo && show_demo'

alias show_trace='emacs $CONCURRIT_HOME/work/trace.txt'

alias copy_finfile1='cp -f $CONCURRIT_HOME/bench/demo/finfile1.txt $CONCURRIT_HOME/bench/demo/finfile.txt'
alias copy_finfile2='cp -f $CONCURRIT_HOME/bench/demo/finfile2.txt $CONCURRIT_HOME/bench/demo/finfile.txt'

alias copy_testfile='cp -f $CONCURRIT_HOME/bench/demo/src/boundedBufferTest_${TEST}.cpp $CONCURRIT_HOME/bench/demo/src/boundedBufferTest.cpp'

export TEST=SearchSeq

alias copy_testfile='cp -f $CONCURRIT_HOME/bench/demo/src/boundedBufferTest_${TEST}.cpp $CONCURRIT_HOME/bench/demo/src/boundedBufferTest.cpp'

alias SearchSeq='copy_finfile1 && export TEST="SearchSeq" && copy_testfile && run_test'
alias SearchAll='copy_finfile1 && export TEST="SearchAll" && copy_testfile && run_test'
alias SearchInFunc='copy_finfile2 && export TEST="SearchInFunc" && copy_testfile && run_test -p0'
alias SearchInFuncRefined='copy_finfile2 && export TEST="SearchInFuncRefined" && copy_testfile && run_test'
alias BuggySchedule='copy_finfile2 && export TEST="BuggySchedule" && copy_testfile && run_test'

# alias run_test_log='GLOG_v=1 run_test'

# alias run_test_no_pin='run_test -p0 -m1'
