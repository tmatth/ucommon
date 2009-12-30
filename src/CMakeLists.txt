# Copyright (C) 2009 David Sugar, Tycho Softworks
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

set(LIBS ${LIBS} ${UCOMMON_LIBS})
add_library(ucommon SHARED 
	object.cpp linked.cpp string.cpp mapped.cpp shell.cpp 
	counter.cpp  bitmap.cpp timer.cpp memory.cpp socket.cpp 
	access.cpp thread.cpp  fsys.cpp cpr.cpp vector.cpp xml.cpp 
	stream.cpp persist.cpp keydata.cpp)
set_target_properties(ucommon PROPERTIES VERSION ${VERSION} SOVERSION ${SOVERSION})
target_link_libraries(ucommon ${LIBS})
install(TARGETS ucommon DESTINATION lib${LIB64})
message("src libraries: ${LIBS}")

