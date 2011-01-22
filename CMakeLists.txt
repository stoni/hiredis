cmake_minimum_required (VERSION 2.6)
project (hiredis)

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")

set (SRC_LIST async.c dict.c hiredis.c sds.c ae/ae.c)

if(WIN32)
	set (SRC_LIST ${SRC_LIST} net_w32.c)
	set (LIB_LIST Ws2_32.lib)
else(WIN32)
	set (SRC_LIST ${SRC_LIST} net.c)
endif(WIN32)

# add library
add_library(libhiredis ${SRC_LIST})

# add tests
add_executable(hiredis-test test.c)
add_executable(example-ae example-ae.c)

#link executables
target_link_libraries(hiredis-test libhiredis ${LIB_LIST})
target_link_libraries(example-ae libhiredis ${LIB_LIST})
