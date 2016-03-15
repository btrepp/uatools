cmake_minimum_required(VERSION 3.3)
project(uatools C)

# Platform-dependant optimization and settings
set(
    CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -ggdb"
    )

include_directories(include)

find_library(OPEN62541 open62541)

#my executable :)
add_executable(uaconnect src/uaconnect.c )
target_link_libraries(uaconnect ${OPEN62541})

install(TARGETS uaconnect DESTINATION bin)
