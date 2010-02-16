project(test)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} 
                    ${CMAKE_CURRENT_SOURCE_DIR}/../inc) 

link_directories(${CMAKE_CURRENT_BINARY_DIR}
		 ${CMAKE_CURRENT_BINARY_DIR}/../src)

add_executable(ucommonLinked linked.cpp)
target_link_libraries(ucommonLinked ucommon)

add_executable(ucommonSocket socket.cpp)
target_link_libraries(ucommonSocket ucommon)

add_executable(ucommonStrings string.cpp)
target_link_libraries(ucommonStrings ucommon)

add_executable(ucommonThreads thread.cpp)
target_link_libraries(ucommonThreads ucommon)

add_executable(ucommonKeydata keydata.cpp)
target_link_libraries(ucommonKeydata ucommon)

add_executable(ucommonStream stream.cpp)
target_link_libraries(ucommonStream ucommon)

