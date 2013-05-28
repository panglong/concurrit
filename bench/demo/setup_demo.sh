# This file sets up the environment for the Concurrit demo.

alias setup_demo='source $CONCURRIT_HOME/bench/demo/setup_demo.sh && compile_demo'

alias compile_demo='$CONCURRIT_HOME/scripts/compile_bench.sh demo'

alias run_test='$CONCURRIT_HOME/scripts/run_bench.sh demo -c -s -t'

alias show_trace='emacs $CONCURRIT_HOME/work/trace.txt'

alias copy_finfile1='cp -f $CONCURRIT_HOME/bench/demo/finfile1.txt $CONCURRIT_HOME/bench/demo/finfile.txt'
alias copy_finfile2='cp -f $CONCURRIT_HOME/bench/demo/finfile2.txt $CONCURRIT_HOME/bench/demo/finfile.txt'

export TEST=SearchSeq

alias copy_testfile='cp -f $CONCURRIT_HOME/bench/demo/src/boundedBufferTest_${TEST}.cpp $CONCURRIT_HOME/bench/demo/src/boundedBufferTest.cpp'

alias SearchSeq='copy_finfile1 && export TEST="SearchSeq" && copy_testfile && run_test'
alias SearchAll='copy_finfile1 && export TEST="SearchAll" && copy_testfile && run_test'
alias SearchInFunc='copy_finfile2 && export TEST="SearchInFunc" && copy_testfile && run_test'
alias SearchInFuncRefined='copy_finfile2 && export TEST="SearchInFuncRefined" && copy_testfile && run_test'
alias BuggySchedule='copy_finfile2 && export TEST="BuggySchedule" && copy_testfile && run_test'

# alias run_test_log='GLOG_v=1 run_test'

# alias run_test_no_pin='run_test -p0 -m1'
