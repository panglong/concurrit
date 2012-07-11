all:
	@echo "Please choose target:"; \
	    echo "build - initial build of apache server, *DO THIS FIRST*"; \
	    echo "        Apache will be configured to listen on port 8091"; \
	    echo "normal - no sleep, bug will occur very rarely"; \
	    echo "sleep - sleeps and logic inserted to make bug more likely"; \
	    echo "fuzz - *just* randomized sleeps inserted to make bug more likely (less reproducible than sleep)"

normal: apache-normal run-info

sleep: apache-sleep run-info

fuzz: apache-fuzz run-info

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
	    tar xjf httpd-2.2.9.tar.bz2 && \
	    cd httpd-2.2.9 && \
	    ./configure --prefix=`pwd`/../../bin/install/ --with-mpm=worker --enable-cache --enable-mem-cache && \
	    make && \
	    make install && \
	    cd ../.. && \
	    sed s/Listen\ 80/Listen\ 8091/ < bin/install/conf/httpd.conf > bin/install/conf/tmp.conf && \
	    mv bin/install/conf/tmp.conf bin/install/conf/httpd.conf


assert:
	stat bin/install && \
	    cd src && \
	    cd httpd-2.2.9 && \
	    cp ../my-mods/fdqueue-assert.c server/mpm/worker/fdqueue.c && \
	    make && \
	    make install
	    
apache-sleep:
	stat bin/install && \
	    cd src && \
	    cd httpd-2.2.9 && \
	    cp ../my-mods/fdqueue-sleep.c server/mpm/worker/fdqueue.c && \
	    make && \
	    make install

apache-fuzz:
	stat bin/install && \
	    cd src && \
	    cd httpd-2.2.9 && \
	    cp ../my-mods/fdqueue-noise.c server/mpm/worker/fdqueue.c && \
	    make && \
	    make install

apache-normal:
	stat bin/install && \
	    cd src && \
	    cd httpd-2.2.9 && \
	    cp ../my-mods/fdqueue-original.c server/mpm/worker/fdqueue.c && \
	    make && \
	    make install

run-info:
	@echo "" ;
	@echo "Execute server with the following command:"; 
	@echo "  /home/najalber/mt-testing/Benchmarks/bug11/bin/install/bin/httpd -DNO_DETACH -DFOREGROUND";
	@echo "";
	@echo "Execute test harness with the following command:";
	@echo " `pwd`/bin/install/bin/ab -c 100 -t 30 http://localhost:8091/";
	@echo "Execute server with the following command:" > bin/run;
	@echo "  /home/najalber/mt-testing/Benchmarks/bug11/bin/install/bin/httpd -DNO_DETACH -DFOREGROUND" >> bin/run;
	@echo "" >> bin/run;
	@echo "Execute test harness with the following command:" >> bin/run;
	@echo " `pwd`/bin/install/bin/ab -c 100 -t 30 http://localhost:8091/" >> bin/run;


clean:
	rm -rf src/apr-1.4.2/ src/httpd-2.2.9 bin/
