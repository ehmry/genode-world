#ifndef INCLUDE_features_h__
#define INCLUDE_features_h__

#undef GIT_DEBUG_POOL
#define GIT_TRACE 1
#undef GIT_THREADS
#undef GIT_MSVC_CRTDBG

#define GIT_ARCH_64 1
#undef GIT_ARCH_32

#undef GIT_USE_ICONV
#define GIT_USE_NSEC 1
#define GIT_USE_STAT_MTIM 1
#define GIT_USE_STAT_MTIMESPEC 1
#define GIT_USE_STAT_MTIME_NSEC 1
#undef GIT_USE_FUTIMENS
#undef GIT_USE_REGCOMP_L

#undef GIT_SSH
#define GIT_SSH_MEMORY_CREDENTIALS 1

#undef GIT_GSSAPI
#undef GIT_WINHTTP
#define GIT_CURL 1

/*
 * Prefer OpenSSL to MbedTLS simply because
 * one is already ported and the other is not.
 */
#define GIT_HTTPS 1
#define GIT_OPENSSL 1
#undef GIT_SECURE_TRANSPORT
#undef GIT_MBEDTLS

#define GIT_SHA1_COLLISIONDETECT 1
#undef GIT_SHA1_WIN32
#define GIT_SHA1_COMMON_CRYPTO 1
#define GIT_SHA1_OPENSSL 1
#undef GIT_SHA1_MBEDTLS

#define NO_MMAP 1

#endif
