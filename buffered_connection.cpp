#include "buffered_connection.hpp"

buffered_connection::connection_pool buffered_connection::connections;
buffered_connection::connection_list buffered_connection::closed_connections;