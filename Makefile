EVX_HEADERS = evx/buffer.hpp evx/buffered_connection.hpp evx/evx.hpp
EVX_SOURCE = evx/buffer.cpp evx/buffered_connection.cpp
CLOUDBRIDGE_HEADERS = src/chat_handler.hpp src/connection_finder.hpp src/connection_pool.hpp src/listen_handler.hpp src/util.hpp src/simple_stats_driver.hpp src/state_stats_driver.hpp
CLOUDBRIDGE_SOURCE = src/chat_handler.cpp src/cloudbridge.cpp src/connection_pool.cpp src/connection_finder.cpp src/listen_handler.cpp src/util.cpp src/simple_stats_driver.cpp src/state_stats_driver.cpp

ALL_HEADERS = ${EVX_HEADERS} ${CLOUDBRIDGE_HEADERS}
ALL_SOURCE = ${EVX_SOURCE} ${CLOUDBRIDGE_SOURCE}
ALL_OBJECTS = ${ALL_SOURCE:.cpp=.o}

COMPILE = g++ -c -O3 -I/opt/local/include -Isrc -I.
LINK = g++ -O3 -L/opt/local/lib -lev -ltomcrypt

%.o: %.cpp ${ALL_HEADERS}
	${COMPILE} -o $@ $<

bin/cloudbridge: ${ALL_OBJECTS} ${ALL_HEADERS}
	mkdir -p bin
	${LINK} -o bin/cloudbridge ${ALL_OBJECTS}

.PHONY: all clean
all:
	bin/cloudbridge
	
clean:
	-rm -f bin/cloudbridge ${ALL_OBJECTS}