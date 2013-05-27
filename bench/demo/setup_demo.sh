# This file sets up the environment for the Concurrit demo.

alias setup_demo='source $CONCURRIT_HOME/bench/demo/setup_demo.sh'

alias compile_demo='$CONCURRIT_HOME/scripts/compile_bench.sh demo'

alias run_test='$CONCURRIT_HOME/scripts/run_bench.sh demo -t1'

alias run_test_log='GLOG_v=1 run_test'

alias run_test_no_pin='run_test -p0 -m1'

alias show_trace='emacs $CONCURRIT_HOME/work/trace.txt'