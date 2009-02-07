cloudbridge: cloudbridge.cpp buffered_connection.cpp buffered_connection.hpp buffer.hpp buffer.cpp evx.hpp
	g++ -I/opt/local/include -L/opt/local/lib -lev -o cloudbridge cloudbridge.cpp buffered_connection.cpp buffer.cpp

all:
	cloudbridge