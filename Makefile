cloudbridge: cloudbridge.cpp evx/buffered_connection.cpp evx/buffered_connection.hpp evx/buffer.hpp evx/buffer.cpp evx/evx.hpp
	g++ -I/opt/local/include -L/opt/local/lib -lev -o cloudbridge cloudbridge.cpp evx/buffered_connection.cpp evx/buffer.cpp

all:
	cloudbridge