all:
	@echo "Please choose target:"; \
	    echo "build - initial build of mysql *DO THIS FIRST*"; \
	    echo "normal - build mysql with no sleeps"; \
	    echo "sleep - put sleeps in mysql to make bug more likely"; \
	    echo "clean - remove build"

build:
	cd src && \
	    mkdir -p ../bin && \
	    tar xzf mysql-4.0.12.tar.gz && \
	    cd mysql-4.0.12 && \
	    cp -r ../my-mods/* . && \
	    export CFLAGS="-g -O0 -gdwarf-2 -ldummy -I/home/elmas/concurrit/include -L/home/elmas/concurrit/lib"  && \
	    export CXXFLAGS="-g -O0 -gdwarf-2 -ldummy -I/home/elmas/concurrit/include -L/home/elmas/concurrit/lib -felide-constructors -fno-exceptions -fno-rtti" && \
	    ./configure --with-named-thread-libs="-lpthread" --prefix=`pwd`/../../bin/ --with-debug=full && \
	    make && \
	    make install && \
	    cd ../../; \
	    ./bin/bin/mysql_install_db; \
	    cd bin && \
	    chown -R ${USER} . && \
	    chmod -R 777 . && \
	    mkdir -p binlogdir && \
	    cp ../src/testharness.sh . && \
	    chmod u+x testharness.sh && \
	    cd .. && \
	    ./src/create_table.sh

normal: 
	cd src && \
	    cp sleep-mods/log.cc mysql-4.0.12/sql/ && \
	    cp sleep-mods/sql_insert.cc mysql-4.0.12/sql/ && \
	    cd mysql-4.0.12 && \
	    make && \
	    make install
	    
assert: 
	cd src && \
	    cp sleep-mods/log-assert.cc mysql-4.0.12/sql/log.cc && \
	    cp sleep-mods/sql_insert-assert.cc mysql-4.0.12/sql/sql_insert.cc && \
	    cd mysql-4.0.12 && \
	    make && \
	    make install
	    
sleep: 
	cd src && \
	    cp sleep-mods/log-sleep.cc mysql-4.0.12/sql/log.cc && \
	    cp sleep-mods/sql_insert-sleep.cc mysql-4.0.12/sql/sql_insert.cc && \
	    cd mysql-4.0.12 && \
	    make && \
	    make install

clean:
	rm -rf src/mysql-4.0.12/ bin
