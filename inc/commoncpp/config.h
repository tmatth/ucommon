#ifndef COMMONCPP_CONFIG_H_
#define COMMONCPP_CONFIG_H_

#ifndef _UCOMMON_UCOMMON_H_
#include <ucommon/ucommon.h>
#endif

#include <streambuf>
#include <iostream>

#define CCXX_NAMESPACES
#define COMMONCPP_NAMESPACE ost
#define NAMESPACE_COMMONCPP namespace ost {
#define TIMEOUT_INF ucommon::Timer::inf

#ifdef  _UCOMMON_EXTENDED_
#define CCXX_EXCEPTIONS
#endif

#ifdef  AF_INET6
#define CCXX_IPV6
#endif

#ifdef  AF_INET
#define CCXX_IPV4
#endif

typedef pthread_t   cctid_t;
typedef int8_t      int8;
typedef uint8_t     uint8;
typedef int16_t     int16;
typedef uint16_t    uint16;
typedef int32_t     int32;
typedef uint32_t    uint32;
typedef int64_t     int64;
typedef uint64_t    uint64;

#endif
