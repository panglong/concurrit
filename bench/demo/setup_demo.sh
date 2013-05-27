# This file sets up the environment for the Concurrit demo.

alias setup_demo='source $CONCURRIT_HOME/bench/demo/setup_demo.sh'

alias compile_demo='$CONCURRIT_HOME/scripts/compile_bench.sh demo'

alias run_demo='$CONCURRIT_HOME/scripts/run_bench.sh demo -u'

alias run_test='$CONCURRIT_HOME/scripts/run_bench.sh demo -c -s'

alias run_test_log='GLOG_v=1 $CONCURRIT_HOME/scripts/run_bench.sh demo -c -s'