EVX_HEADERS = evx/buffer.hpp evx/buffered_connection.hpp evx/evx.hpp
EVX_SOURCE = evx/buffer.cpp evx/buffered_connection.cpp
CLOUDBRIDGE_HEADERS = chat_handler.hpp connection_finder.hpp listen_handler.hpp
CLOUDBRIDGE_SOURCE = chat_handler.cpp cloudbridge.cpp connection_finder.cpp listen_handler.cpp

ALL_HEADERS = ${EVX_HEADERS} ${CLOUDBRIDGE_HEADERS}
ALL_SOURCE = ${EVX_SOURCE} ${CLOUDBRIDGE_SOURCE}
ALL = ${ALL_HEADERS} ${ALL_SOURCE}

cloudbridge: ${ALL}
	g++ -I/opt/local/include -L/opt/local/lib -lev -o cloudbridge ${ALL_SOURCE}

all:
	cloudbridge