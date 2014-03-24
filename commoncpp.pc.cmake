prefix=${CMAKE_INSTALL_PREFIX}
libdir=${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}
includedir=${CMAKE_INSTALL_PREFIX}/include
modflags="${MODULE_FLAGS}"

Name: commoncpp
Description: GNU Common C++ compat class framework
Version: ${PACKAGE_FILE_VERSION}
Libs:  -lcommoncpp -lucommon ${PACKAGE_LIBS}
Cflags: ${PACKAGE_FLAGS}
