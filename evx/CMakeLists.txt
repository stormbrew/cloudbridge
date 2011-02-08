set (evx_VERSION_MAJOR 1)
set (evx_VERSION_MINOR 0)

include_directories(include)
add_library(evx
    include/evx.hpp

    include/evx/buffer.hpp
    src/buffer.cpp

    include/evx/buffered_connection.hpp
    src/buffered_connection.cpp
)
