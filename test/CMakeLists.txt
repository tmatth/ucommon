project(test)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/../inc)

link_directories(${CMAKE_CURRENT_BINARY_DIR}
         ${CMAKE_CURRENT_BINARY_DIR}/../src)

add_executable(test-ucommonLinked linked.cpp)
target_link_libraries(test-ucommonLinked ${UCOMMON_DEPEND})
add_test(NAME ucommonLinked COMMAND test-ucommonLinked)

add_executable(test-ucommonSocket socket.cpp)
target_link_libraries(test-ucommonSocket ${UCOMMON_DEPEND})
add_test(NAME ucommonSocket COMMAND test-ucommonSocket)

add_executable(test-ucommonStrings string.cpp)
target_link_libraries(test-ucommonStrings ${UCOMMON_DEPEND})
add_test(NAME ucommonStrings COMMAND test-ucommonStrings)

add_executable(test-ucommonThreads thread.cpp)
target_link_libraries(test-ucommonThreads ${UCOMMON_DEPEND})
add_test(NAME ucommonThreads COMMAND test-ucommonThreads)

add_executable(test-ucommonKeydata keydata.cpp)
target_link_libraries(test-ucommonKeydata ${UCOMMON_DEPEND})
add_test(NAME ucommonKeydata COMMAND test-ucommonKeydata)

add_executable(test-ucommonStream stream.cpp)
target_link_libraries(test-ucommonStream ${UCOMMON_DEPEND})
add_test(NAME ucommonStream COMMAND test-ucommonStream)

add_executable(test-ucommonUnicode unicode.cpp)
target_link_libraries(test-ucommonUnicode ${UCOMMON_DEPEND})
add_test(NAME ucommonUnicode COMMAND test-ucommonUnicode)

add_executable(test-ucommonQueue queue.cpp)
target_link_libraries(test-ucommonQueue ${UCOMMON_DEPEND})
add_test(NAME ucommonQueue COMMAND test-ucommonQueue)

add_executable(test-ucommonDatetime datetime.cpp)
target_link_libraries(test-ucommonDatetime ${UCOMMON_DEPEND})
add_test(NAME ucommonDatetime COMMAND test-ucommonDatetime)

add_executable(test-ucommonShell shell.cpp)
target_link_libraries(test-ucommonShell ${UCOMMON_DEPEND})
add_test(NAME ucommonShell COMMAND test-ucommonShell)

add_executable(test-ucommonCipher cipher.cpp)
target_link_libraries(test-ucommonCipher ${USECURE_DEPEND} ${UCOMMON_DEPEND})
add_test(NAME ucommonCipher COMMAND test-ucommonCipher)

add_executable(test-ucommonDigest digest.cpp)
target_link_libraries(test-ucommonDigest ${USECURE_DEPEND} ${UCOMMON_DEPEND})
add_test(NAME ucommonDigest COMMAND test-ucommonDigest)

