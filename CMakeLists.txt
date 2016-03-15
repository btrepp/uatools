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
add_executable(uaendpoints src/uaendpoints.c )
add_executable(uabrowse src/uabrowse.c )
add_executable(uaread src/uaread.c )
target_link_libraries(uaconnect ${OPEN62541})
target_link_libraries(uaendpoints ${OPEN62541})
target_link_libraries(uabrowse ${OPEN62541})
target_link_libraries(uaread ${OPEN62541})

install(TARGETS uaconnect DESTINATION bin)
install(TARGETS uaendpoints DESTINATION bin)
install(TARGETS uabrowse DESTINATION bin)
install(TARGETS uaread DESTINATION bin)
