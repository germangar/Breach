
#ifndef __Q_ARCH_H__
#define __Q_ARCH_H__

#ifdef __cplusplus
extern "C" {
#endif

// global switches
#include "config.h"

// q_shared.h -- included first by ALL program modules
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

//==============================================

#ifdef _WIN32

// unknown pragmas are SUPPOSED to be ignored, but....
#pragma warning( disable : 4244 )       // MIPS
#pragma warning( disable : 4136 )       // X86
#pragma warning( disable : 4051 )       // ALPHA

//# pragma warning(disable : 4018)		// signed/unsigned mismatch
//# pragma warning(disable : 4305)		// truncation from const double to float
#pragma warning( disable : 4514 )       // unreferenced inline function has been removed
#pragma warning( disable : 4152 )       // nonstandard extension, function/data pointer conversion in expression
#pragma warning( disable : 4201 )       // nonstandard extension used : nameless struct/union
#pragma warning( disable : 4054 )       // 'type cast' : from function pointer to data pointer
#pragma warning( disable : 4127 )       // conditional expression is constant
#pragma warning( disable : 4100 )       // unreferenced formal parameter
#pragma warning( disable : 4706 )       // assignment within conditional expression
#pragma warning( disable : 4702 )       // unreachable code
#pragma warning( disable : 4306 )       // conversion from 'int' to 'void *' of greater size
#pragma warning( disable : 4305 )       // truncation from 'void *' to 'int'

#if defined _M_AMD64
#pragma warning( disable : 4267 )       // conversion from 'size_t' to whatever, possible loss of data
#endif

#define HAVE___INLINE

#define HAVE__SNPRINTF

#define HAVE__VSNPRINTF

#define HAVE__STRICMP

#ifdef LCC_WIN32
#ifndef C_ONLY
#define C_ONLY
#endif
#define HAVE_TCHAR
#define HAVE_MMSYSTEM
#define HAVE_DLLMAIN
#else
#define HAVE_WSIPX
#endif

#define LIB_DIRECTORY "libs"
#define LIB_SUFFIX ".dll"

#define VID_INITFIRST

#define GL_DRIVERNAME "opengl32.dll"

#define VORBISFILE_LIBNAME "libvorbisfile.dll"

#ifdef NDEBUG
#define BUILDSTRING "Win32 RELEASE"
#else
#define BUILDSTRING "Win32 DEBUG"
#endif

#ifdef _M_IX86
#if defined __FreeBSD__
#define CPUSTRING "i386"
#define ARCH "freebsd_i386"
#else
#define CPUSTRING "x86"
#define ARCH "x86"
#endif
#elif defined ( _M_AMD64 )
#if defined __FreeBSD__
#define CPUSTRING "x86_64"
#define ARCH "freebsd_x86_64"
#else
#define CPUSTRING "x64"
#define ARCH "x64"
#endif
#elif defined ( _M_ALPHA )
#define CPUSTRING "axp"
#define ARCH	  "axp"
#endif

// doh, some compilers need a _ prefix for variables so they can be
// used in asm code
#ifdef __GNUC__     // mingw
#define VAR( x )    "_" # x
#else
#define VAR( x )    # x
#endif

#ifdef _MSC_VER
#define HAVE___CDECL
#endif

// 64bit integers and integer-pointer types
#include <basetsd.h>
typedef INT64 qint64;
typedef UINT64 quint64;
typedef INT_PTR qintptr;
typedef UINT_PTR quintptr;

#endif

//==============================================

#if defined ( __linux__ ) || defined ( __FreeBSD__ )

#define HAVE_INLINE

#ifndef HAVE_STRCASECMP // SDL_config.h seems to define this too...
#define HAVE_STRCASECMP
#endif

#define LIB_DIRECTORY "libs"
#define LIB_SUFFIX ".so"

//# define GL_FORCEFINISH
#define GL_DRIVERNAME  "libGL.so.1"

#define VORBISFILE_LIBNAME "libvorbisfile.so"

#ifdef __FreeBSD__
#define BUILDSTRING "FreeBSD"
#else
#define BUILDSTRING "Linux"
#endif

#ifdef __i386__
#if defined __FreeBSD__
#define ARCH "freebsd_i386"
#define CPUSTRING "i386"
#else
#define ARCH "i386"
#define CPUSTRING "i386"
#endif
#elif defined ( __x86_64__ )
#if defined __FreeBSD__
#define ARCH "freebsd_x86_64"
#define CPUSTRING "x86_64"
#else
#define ARCH "x86_64"
#define CPUSTRING "x86_64"
#endif
#elif defined ( __powerpc__ )
#define CPUSTRING "ppc"
#define ARCH "ppc"
#elif defined ( __alpha__ )
#define CPUSTRING "axp"
#define ARCH "axp"
#else
#define CPUSTRING "Unknown"
#define ARCH "Unknown"
#endif

#define VAR( x ) # x

// 64bit integers and integer-pointer types
#include <stdint.h>
typedef int64_t qint64;
typedef uint64_t quint64;
typedef intptr_t qintptr;
typedef uintptr_t quintptr;

#endif

//==============================================

#if defined ( __APPLE__ ) || defined ( MACOSX )

#define HAVE_INLINE

#define HAVE_STRCASECMP

//# define GL_FORCEFINISH
#define GL_DRIVERNAME  "libGL.dylib"

#define VORBISFILE_LIBNAME "libvorbisfile.dylib"

#define BUILDSTRING "MacOSX"

#ifdef __i386__
#define CPUSTRING "i386"
#elif defined ( __ppc__ )
#define CPUSTRING "ppc"
#else
#define CPUSTRING "Unknown"
#endif

#if defined __FreeBSD__
#if defined __i386__
#define ARCH "freebsd_i386"
#define CPUSTRING "i386"
#elif defined __x86_64__
#define ARCH "freebsd_x86_64"
#define CPUSTRING "x86_64"
#endif
#elif defined __i386__
#define ARCH "i386"
#define CPUSTRING "i386"
#elif defined __x86_64__
#define ARCH "x86_64"
#define CPUSTRING "x86_64"
#elif defined __alpha__
#define ARCH "axp"
#define CPUSTRING "axp"
#elif defined __powerpc__
#define ARCH "ppc"
#define CPUSTRING "ppc"
#elif defined __sparc__
#define ARCH "sparc"
#define CPUSTRING "sparc"
#else
#error "Unknown ARCH"
#endif

#define VAR( x ) # x

#endif

//==============================================

#ifdef HAVE___INLINE
#ifndef inline
#define inline __inline
#endif
#elif !defined ( HAVE_INLINE )
#ifndef inline
#define inline
#endif
#endif

#ifdef HAVE__SNPRINTF
#ifndef snprintf
#define snprintf _snprintf
#endif
#endif

#ifdef HAVE__VSNPRINTF
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif
#endif

#ifdef HAVE__STRICMP
#ifndef Q_stricmp
#define Q_stricmp( s1, s2 ) _stricmp( ( s1 ), ( s2 ) )
#endif
#ifndef Q_strnicmp
#define Q_strnicmp( s1, s2, n ) _strnicmp( ( s1 ), ( s2 ), ( n ) )
#endif
#endif

#ifdef HAVE_STRCASECMP
#ifndef Q_stricmp
#define Q_stricmp( s1, s2 ) strcasecmp( ( s1 ), ( s2 ) )
#endif
#ifndef Q_strnicmp
#define Q_strnicmp( s1, s2, n ) strncasecmp( ( s1 ), ( s2 ), ( n ) )
#endif
#endif

#if ( defined ( _M_IX86 ) || defined ( __i386__ ) || defined ( __ia64__ ) ) && !defined ( C_ONLY )
#define id386
#else
#ifdef id386
#undef id386
#endif
#endif

#ifndef BUILDSTRING
#define BUILDSTRING "NON-WIN32"
#endif

#ifndef CPUSTRING
#define CPUSTRING  "NON-WIN32"
#endif

#ifdef HAVE_TCHAR
#include <tchar.h>
#endif

#ifdef HAVE___CDECL
#define qcdecl __cdecl
#else
#define qcdecl
#endif

#if defined ( __GNUC__ )
#define ALIGN( x )   __attribute__( ( aligned( x ) ) )
#elif defined ( _MSC_VER )
#define ALIGN( x )   __declspec( align( x ) )
#else
#define ALIGN( x )
#endif

//==============================================

typedef unsigned char qbyte;
typedef enum { qfalse, qtrue }	  qboolean;

#ifndef NULL
#define NULL ( (void *)0 )
#endif

#ifdef __cplusplus
};
#endif

#endif // __Q_ARCH_H__
