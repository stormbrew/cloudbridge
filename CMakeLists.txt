cmake_minimum_required (VERSION 2.6)
project (cloudbridge)

set (cloudbridge_VERSION_MAJOR 1)
set (cloudbridge_VERSION_MINOR 0)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

find_package(TomCrypt REQUIRED)
include_directories(${TomCrypt_INCLUDE_DIRS})
set(LIBS ${LIBS} ${TomCrypt_LIBRARIES})

find_package(Ev REQUIRED)
include_directories(${Ev_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Ev_LIBRARIES})

include_directories ("${PROJECT_SOURCE_DIR}/evx")
add_subdirectory(evx)
set(LIBS ${LIBS} evx)
include_directories(evx/include)

configure_file (
  "${PROJECT_SOURCE_DIR}/include/config.hpp.in"
  "${PROJECT_BINARY_DIR}/build/config.hpp"
  )

include_directories(include build)

add_executable (cloudbridge 
    include/chat_handler.hpp
    src/chat_handler.cpp

    include/connection_pool.hpp
    src/connection_pool.cpp

    include/connection_finder.hpp
    src/connection_finder.cpp

    include/listen_handler.hpp
    src/listen_handler.cpp

    include/util.hpp
    src/util.cpp

    include/simple_stats_driver.hpp
    src/simple_stats_driver.cpp

    include/state_stats_driver.hpp
    src/state_stats_driver.cpp

    src/cloudbridge.cpp
)
target_link_libraries (cloudbridge ${LIBS})
