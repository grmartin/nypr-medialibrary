m4_define([MEDIALIB_MAJOR], [0])
m4_define([MEDIALIB_MINOR], [1])
m4_define([MEDIALIB_MICRO], [0])

AC_COPYRIGHT([Copyright 2014-2016 Hugo Beauzée-Luyssen - VideoLabs])

AC_INIT([medialibrary], [MEDIALIB_MAJOR.MEDIALIB_MINOR.MEDIALIB_MICRO])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [
  AM_SILENT_RULES([yes])
])
AC_CANONICAL_HOST

MEDIALIBRARY_MAJOR=MEDIALIB_MAJOR
MEDIALIBRARY_MINOR=MEDIALIB_MINOR
MEDIALIBRARY_MICRO=MEDIALIB_MICRO
AC_SUBST(MEDIALIBRARY_MAJOR)
AC_SUBST(MEDIALIBRARY_MINOR)
AC_SUBST(MEDIALIBRARY_MICRO)

AC_USE_SYSTEM_EXTENSIONS

LT_INIT([win32-dll shared disable-static])
AC_PROG_CXX
AC_PROG_OBJCXX
AX_CXX_COMPILE_STDCXX_11([noext])

AC_CONFIG_MACRO_DIR([m4])

AC_HEADER_ASSERT

dnl
dnl  Check the operating system
dnl
HAVE_WIN64="0"
HAVE_IOS="0"
HAVE_OSX="0"
HAVE_TVOS="0"

case "${host_os}" in
  "")
    SYS=unknown
    ;;
  linux*)
    SYS=linux
    ;;
  bsdi*)
    SYS=bsdi
    ;;
  freebsd*)
    SYS=freebsd
    CPPFLAGS="${CPPFLAGS} -I/usr/local/include"
    LDFLAGS="${LDFLAGS} -L/usr/local/lib"
    ;;
  openbsd*)
    SYS=openbsd
    CPPFLAGS="${CPPFLAGS} -I/usr/local/include -I/usr/X11R6/include"
    LDFLAGS="${LDFLAGS} -L/usr/local/lib -L/usr/X11R6/lib/"
    ;;
  netbsd*)
    SYS=netbsd
    ;;
  dragonfly*)
    SYS=dragonfly
    ;;
  darwin*)

    dnl Force gcc "-arch" flag
    ARCH_flag=""
    case "${host}" in
      i?86*)
        ARCH_flag="-arch i386"
      ;;
      ppc64*)
        ARCH_flag="-arch ppc64"
      ;;
      ppc*)
        ARCH_flag="-arch ppc"
      ;;
      x86_64*)
        ARCH_flag="-arch x86_64"
      ;;
      arm*)
        ac_cv_c_bigendian="no"
        ac_cv_c_attribute_packed="no"
      ;;
    esac

    SYS=darwin
    CFLAGS="${CFLAGS} -D_INTL_REDIRECT_MACROS -std=gnu11 ${ARCH_flag}"
    CXX="${CXX} -stdlib=libc++ -std=c++11"
    CXXFLAGS="${CXXFLAGS} -D_INTL_REDIRECT_MACROS ${ARCH_flag}"
    CPPFLAGS="${CPPFLAGS} ${ARCH_flag}"
    OBJCXX="${OBJCXX} -stdlib=libc++ -std=c++11"
    OBJCXXFLAGS="${OBJCXXFLAGS} -D_INTL_REDIRECT_MACROS ${ARCH_flag}"
    LDFLAGS="${LDFLAGS} -Wl,-headerpad_max_install_names ${ARCH_flag}"

    AC_EGREP_CPP(yes,
            [#import <TargetConditionals.h>
             #if TARGET_OS_IPHONE
             yes
             #endif],
             [HAVE_IOS="1"],
             [HAVE_OSX="1"])

    AC_EGREP_CPP(yes,
            [#import <TargetConditionals.h>
             #if TARGET_OS_TV
             yes
             #endif],
             [HAVE_TVOS="1"
             HAVE_IOS="0"
             HAVE_OSX="0"
             ],)

    dnl Allow binaries created on Lion to run on earlier releases
    AC_EGREP_CPP(yes,
           [#import <Cocoa/Cocoa.h>
            #ifdef MAC_OS_X_VERSION_10_7
            yes
            #endif],
            [AC_MSG_RESULT([yes])
            AC_LIBOBJ([getdelim])
            AC_LIBOBJ([strndup])
            AC_LIBOBJ([strnlen])],)

    dnl
    dnl  Handle Mac OS X SDK flags
    dnl
    AC_ARG_WITH(macosx-sdk,
      [AS_HELP_STRING([--with-macosx-sdk=DIR],
        [compile using the SDK in DIR])])
    if test "${with_macosx_sdk}" != "" ; then
        test ! -d "${with_macosx_sdk}" && AC_MSG_ERROR([SDK "${with_macosx_sdk}" not found])
        CPP="${CPP} -isysroot ${with_macosx_sdk}"
        CC="${CC} -isysroot ${with_macosx_sdk}"
        CXX="${CXX} -isysroot ${with_macosx_sdk}"
        OBJC="${OBJC} -isysroot ${with_macosx_sdk}"
        OBJCXX="${OBJCXX} -isysroot ${with_macosx_sdk}"
        LD="${LD} -syslibroot ${with_macosx_sdk}"
    fi
    AC_ARG_WITH(macosx-version-min,
      [AS_HELP_STRING([--with-macosx-version-min=VERSION],
        [compile for Mac OS X VERSION and above])])
    if test "${with_macosx_version_min}" != "" ; then
        CPP="${CPP} -mmacosx-version-min=${with_macosx_version_min}"
        CC="${CC} -mmacosx-version-min=${with_macosx_version_min}"
        CXX="${CXX} -mmacosx-version-min=${with_macosx_version_min}"
        OBJC="${OBJC} -mmacosx-version-min=${with_macosx_version_min}"
        OBJCXX="${OBJCXX} -mmacosx-version-min=${with_macosx_version_min}"
        LD="${LD} -mmacosx_version_min=${with_macosx_version_min}"
        OBJCXXLD="${OBJCXXLD} -mmacosx_version_min=${with_macosx_version_min}"
    fi
    ;;
  *mingw32* | *cygwin* | *wince* | *mingwce*)
    CXXFLAGS="${CXXFLAGS} -static-libgcc -static-libstdc++"
    AC_DEFINE([_UNICODE], [1], [Define to 1 for Unicode (Wide Chars) APIs.])
    AC_DEFINE([UNICODE], [1], [Define to 1 for Unicode (Wide Chars) APIs.])
    AC_DEFINE([_POSIX_C_SOURCE], [200809L], [IEEE Std 1003.1.])
    AC_DEFINE([_BSD_SOURCE], [1], [ISO C, POSIX, and 4.3BSD things.])

    case "${host_os}" in
      *mingw32*)
        SYS=mingw32
        ;;
      *cygwin*)
        SYS=cygwin
        ;;
    esac

    if test "${SYS}" = "mingw32"; then
        # DEP, ASLR, NO SEH
        LDFLAGS="${LDFLAGS} -Wl,--nxcompat -Wl,--no-seh -Wl,--dynamicbase"

        AC_CHECK_PROGS(U2D, [unix2dos todos], unix2dos)
        ac_default_prefix="`pwd`/_win32"
        DESTDIR="`pwd`/_win32/"

        dnl
        dnl NSIS/MSI Installer prefix and WIN64
        dnl
        case "${host}" in
            amd64*|x86_64*)
                HAVE_WIN64="1"
                WINDOWS_ARCH="x64"
                PROGRAMFILES="PROGRAMFILES64"
            ;;
            *)
                WINDOWS_ARCH="x86"
                PROGRAMFILES="PROGRAMFILES"
            ;;
        esac
        AC_SUBST(WINDOWS_ARCH)
        AC_SUBST(PROGRAMFILES)

    fi
    ;;
  *nto*)
    SYS=nto
    ;;
  solaris*)
    SYS=solaris
    ;;
  hpux*)
    SYS=hpux
    ;;
  *os2*)
    SYS=os2
    LDFLAGS="${LDFLAGS} -Zomf -Zbin-files -Zargs-wild -Zhigh-mem"
    AC_LIBOBJ([freeaddrinfo])
    ;;
  *)
    SYS="${host_os}"
    ;;
esac
AM_CONDITIONAL(HAVE_WIN32,   test "${SYS}" = "mingw32")
AM_CONDITIONAL(HAVE_LINUX,   test "${SYS}" = "linux")
AM_CONDITIONAL(HAVE_DARWIN,  test "${SYS}" = "darwin")
AM_CONDITIONAL(HAVE_IOS,     test "${HAVE_IOS}" = "1")
AM_CONDITIONAL(HAVE_OSX,     test "${HAVE_OSX}" = "1")
AM_CONDITIONAL(HAVE_TVOS,    test "${HAVE_TVOS}" = "1")

AC_LANG_PUSH([C++])
AC_MSG_CHECKING([c++11 thread support])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([#include <thread>
                   int main() { std::thread t; }])], [
        AC_MSG_RESULT([ok])
        AC_DEFINE(CXX11_THREADS, 1, [Defined to 1 if C++11 threads are supported])
    ],[ AC_MSG_RESULT([no])])
AC_LANG_POP([C++])

AC_LANG_PUSH([C++])
AC_MSG_CHECKING([c++11 std::mutex support])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([#include <mutex>
                   int main() { std::mutex m; }])], [
        AC_MSG_RESULT([ok])
        AC_DEFINE(CXX11_MUTEX, 1, [Defined to 1 if C++11 std::mutex is supported])
    ],[ AC_MSG_RESULT([no])])
AC_LANG_POP([C++])

AC_LANG_PUSH([C++])
AC_MSG_CHECKING([c++11 thread_local support])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([thread_local int seaOtter = 0;
                   int main() {}])], [
        AC_MSG_RESULT([ok])
        AC_DEFINE(CXX11_THREAD_LOCAL, 1, [Defined to 1 if C++11 thread_local storage qualifier is supported])
    ],[ AC_MSG_RESULT([no])])
AC_LANG_POP([C++])

dnl Android is linux, but a bit different
AS_IF([test "$SYS" = linux],[
    AC_MSG_CHECKING([for an Android system])
    AC_PREPROC_IFELSE([AC_LANG_PROGRAM(
                      [[#ifndef __ANDROID__
                       # error Not Android
                       #endif
                       ]],[[;]])
        ],[
            HAVE_ANDROID="1"
            AC_MSG_RESULT([yes])
        ],[AC_MSG_RESULT([no])
    ])
])
AM_CONDITIONAL(HAVE_ANDROID, test "${HAVE_ANDROID}" = "1")

AS_IF([test "$SYS" = mingw32],[
    AH_TOP([#if defined(_WIN32) && !defined(_WIN32_WINNT)])
    AH_TOP([# define _WIN32_WINNT 0x0600 ])
    AH_TOP([#endif])
])

AC_ARG_ENABLE(debug, [AC_HELP_STRING([--enable-debug], [build with run-time assertions (default enabled)])])
AH_TEMPLATE(NDEBUG, [Define to 1 if debug code should NOT be compiled])
AS_IF([test "${enable_debug}" = "yes"],[
  AC_DEFINE(NDEBUG)
])

PKG_CHECK_MODULES(SQLITE, sqlite3)
PKG_CHECK_MODULES(VLC, libvlc >= 3.0)
PKG_CHECK_MODULES(VLCPP, libvlcpp,
    [AC_MSG_RESULT([Found libvlcpp.pc])],
    [
        # Checking for files doesn't work when cross compiling. Simply assume
        # The file is there
        AC_MSG_RESULT([Falling back to submodule])
        AC_SUBST([VLCPP_CFLAGS],[-I${srcdir}/libvlcpp])
    ]
)

PKG_CHECK_MODULES(LIBJPEG, libjpeg, [
     have_libjpeg="yes"
     AC_DEFINE(HAVE_JPEG, 1, [Define to 1 if libjpeg is available])
 ])
AM_CONDITIONAL([HAVE_LIBJPEG], [test "${have_libjpeg}" = "yes"])

AC_ARG_ENABLE(evas, AC_HELP_STRING([--enable-evas], [Use evas to compress thumbnails]))
AS_IF([test "$enable_evas" = "yes"],
    [PKG_CHECK_MODULES(EVAS, evas, [
        have_evas="yes"
        AC_DEFINE(HAVE_EVAS, 1, [Define to 1 if evas is available])
])])

AM_CONDITIONAL([HAVE_EVAS], [test "${have_evas}" = "yes"])

AC_ARG_ENABLE(tests,AC_HELP_STRING([--disable-tests], [Disable build of automated tests suites]))
AM_CONDITIONAL([HAVE_TESTS], [test "${enable_tests}" = "yes"])
AS_IF([test "${enable_tests}" = "yes"], [
    AX_PTHREAD([have_pthread=yes],[have_pthread=no])
    PKG_CHECK_MODULES(RAPIDJSON, RapidJSON, [], [
        # We need the .pc for cross compilation, but we don't consider it a failure
        # when building natively, mostly because debian doesn't package the .pc
        if test "$host" != "$build" ; then
            AC_MSG_ERROR(["Failed to find rapidjson"])
        else
            AC_MSG_WARN("Failed to find rapidjson using pkg-config. Using default location")
        fi
    ])
])
AM_CONDITIONAL([HAVE_PTHREAD], [test "${have_pthread}" = "yes"])

AH_BOTTOM([#include "Fixup.h"])

PKG_INSTALLDIR

AC_CONFIG_FILES([
Makefile
medialibrary.pc
])

AC_OUTPUT
