#if defined(_MSC_VER) || defined(WIN32) || defined(_WIN32)
#ifdef	__EXPORT
#undef	__EXPORT
#endif

#define	__EXPORT __declspec(dllexport)
#endif

