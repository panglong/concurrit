all:
	@echo "Please choose target:"; \
	    echo "build - initial build of apache server, *DO THIS FIRST*"; \
	    echo "        Apache will be configured to listen on port 8090"; \
	    echo "normal - no sleep, bug will occur very rarely"; \
	    echo "sleep - sleeps (and wrapper functions) inserted to make bug more likely"; \
	    echo "assert - tries to fix mislabeling by adding an assert "

normal: apache-normal run-info

sleep: apache-sleep run-info

assert: apache-assert run-info

build:
	cd src && \
	    mkdir -p ../bin/install && \
	    tar xzf apr-1.4.2.tar.gz && \
	    cd apr-1.4.2 && \
	    export CFLAGS="-g -O0 -gdwarf-2 -ldummy -I/home/elmas/concurrit/include -L/home/elmas/concurrit/lib"  && \
	    export CXXFLAGS="-g -O0 -gdwarf-2 -ldummy -I$CONCURRIT_HOME/include -L$CONCURRIT_HOME/lib" && \
	    ./configure --prefix=`pwd`/../../bin/install/ && \
	    make && \
	    make install && \
	    cd .. && \
	    tar xjf httpd-2.2.8.tar.bz2 && \
	    cd httpd-2.2.8 && \
	    cp ../my-mods/mpm_common.c server/ && \
	    ./configure --prefix=`pwd`/../../bin/install/ --with-mpm=worker --enable-cache --enable-mem-cache && \
	    make && \
	    make install && \
	    cd ../.. && \
	    sed s/Listen\ 80/Listen\ 8090/ < bin/install/conf/httpd.conf > bin/install/conf/tmp.conf && \
	    mv bin/install/conf/tmp.conf bin/install/conf/httpd.conf 


apache-sleep:
	stat bin/install && \
	    cd src && \
	    cd httpd-2.2.8 && \
	    cp ../my-mods/fdqueue-sleep.c server/mpm/worker/fdqueue.c && \
	    make && \
	    make install

apache-assert:
	stat bin/install && \
	    cd src && \
	    cd httpd-2.2.8 && \
	    cp ../my-mods/fdqueue-assert.c server/mpm/worker/fdqueue.c && \
	    make && \
	    make install


apache-normal:
	stat bin/install && \
	    cd src && \
	    cd httpd-2.2.8 && \
	    cp ../my-mods/fdqueue-original.c server/mpm/worker/fdqueue.c && \
	    make && \
	    make install

run-info:
	@echo "" ;
	@echo "Execute server with the following command:"; 
	@echo "    MALLOC_CHECK_=3 `pwd`/bin/install/bin/httpd -DNO_DETACH -DFOREGROUND";
	@echo;
	@echo "Execute test harness with the following command:";
	@echo "    `pwd`/scripts/apache_request.sh"
	@echo "Execute server with the following command:" > bin/run
	@echo "    MALLOC_CHECK_=3 `pwd`/bin/install/bin/httpd -DNO_DETACH -DFOREGROUND" >> bin/run;
	@echo "" >> bin/run;
	@echo "Execute test harness with the following command:" >> bin/run;
	@echo "    `pwd`/scripts/apache_request.sh" >> bin/run


clean:
	rm -rf src/apr-1.4.2/ src/httpd-2.2.8 bin/
