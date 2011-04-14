cmake_minimum_required (VERSION 2.6)
project (hiredis)

include_directories("${PROJECT_BINARY_DIR}")

set (SRC_LIST async.c dict.c hiredis.c sds.c ae/ae.c)

if(WIN32)
	set (SRC_LIST ${SRC_LIST} net_w32.c)
	set (LIB_LIST Ws2_32.lib)
else(WIN32)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		add_definitions(-DHAVE_KQUEUE)
	else(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		add_definitions(-DHAVE_EPOLL)
	endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set (SRC_LIST ${SRC_LIST} net.c)
endif(WIN32)

# add library
add_library(hiredis ${SRC_LIST})

# add tests
add_executable(hiredis-test test.c)
add_executable(example-ae example-ae.c)

#link executables
target_link_libraries(hiredis-test hiredis ${LIB_LIST})
target_link_libraries(example-ae hiredis ${LIB_LIST})

#installation
install(TARGETS hiredis-test example-ae hiredis
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib/static)

install(FILES async.h dict.h fmacros.h util.h net.h hiredis.h DESTINATION include)
install(DIRECTORY adapters DESTINATION include/adapters FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ae DESTINATION include/ae FILES_MATCHING PATTERN "*.h")
