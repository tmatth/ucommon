project(test)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/../inc)

link_directories(${CMAKE_CURRENT_BINARY_DIR}
         ${CMAKE_CURRENT_BINARY_DIR}/../src)

add_executable(testing-ucommonLinked linked.cpp)
target_link_libraries(testing-ucommonLinked ucommon)
add_test(NAME ucommonLinked COMMAND testing-ucommonLinked)

add_executable(testing-ucommonSocket socket.cpp)
target_link_libraries(testing-ucommonSocket ucommon)
add_test(NAME ucommonSocket COMMAND testing-ucommonSocket)

add_executable(testing-ucommonStrings string.cpp)
target_link_libraries(testing-ucommonStrings ucommon)
add_test(NAME ucommonStrings COMMAND testing-ucommonStrings)

add_executable(testing-ucommonThreads thread.cpp)
target_link_libraries(testing-ucommonThreads ucommon)
add_test(NAME ucommonThreads COMMAND testing-ucommonThreads)

add_executable(testing-ucommonKeydata keydata.cpp)
target_link_libraries(testing-ucommonKeydata ucommon)
add_test(NAME ucommonKeydata COMMAND testing-ucommonKeydata)

add_executable(testing-ucommonStream stream.cpp)
target_link_libraries(testing-ucommonStream ucommon)
add_test(NAME ucommonStream COMMAND testing-ucommonStream)

add_executable(testing-ucommonUnicode unicode.cpp)
target_link_libraries(testing-ucommonUnicode ucommon)
add_test(NAME ucommonUnicode COMMAND testing-ucommonUnicode)

add_executable(testing-ucommonQueue queue.cpp)
target_link_libraries(testing-ucommonQueue ucommon)
add_test(NAME ucommonQueue COMMAND testing-ucommonQueue)

add_executable(testing-ucommonDatetime datetime.cpp)
target_link_libraries(testing-ucommonDatetime ucommon)
add_test(NAME ucommonDatetime COMMAND testing-ucommonDatetime)

add_executable(testing-ucommonShell shell.cpp)
target_link_libraries(testing-ucommonShell ucommon)
add_test(NAME ucommonShell COMMAND testing-ucommonShell)

add_executable(testing-ucommonCipher cipher.cpp)
target_link_libraries(testing-ucommonCipher usecure ucommon)
add_test(NAME ucommonCipher COMMAND testing-ucommonCipher)

add_executable(testing-ucommonDigest digest.cpp)
target_link_libraries(testing-ucommonDigest usecure ucommon)
add_test(NAME ucommonDigest COMMAND testing-ucommonDigest)

