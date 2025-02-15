# -*- autoconf -*-
#
# Copyright 2011 Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000 with Sandia Corporation, the U.S.  Government
# retains certain rights in this software.
#
# Copyright (c) 2017 Intel Corporation. All rights reserved.
# This software is available to you under the BSD license.
#
# This file is part of the Sandia OpenSHMEM software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

dnl Init Autoconf/Automake/Libtool

AC_INIT([Sandia OpenSHMEM], [1.5.3], [https://github.com/Sandia-OpenSHMEM/SOS/issues], [sandia-openshmem], [https://github.com/Sandia-OpenSHMEM/SOS])
AC_PREREQ([2.60])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])

AM_INIT_AUTOMAKE([foreign check-news color-tests parallel-tests dist-bzip2 no-define subdir-objects tar-ustar 1.11 -Wall -Werror])

m4_ifndef([AS_VAR_COPY],
    [m4_define([AS_VAR_COPY],
       [AS_LITERAL_IF([$1[]$2], [$1=$$2], [eval $1=\$$2])])])

dnl Work around change in how the rules were implemented between
dnl 1.11.2 and 1.11.3.
AM_SILENT_RULES([yes])
AS_IF([test -z "$AM_V"],
  [AM_V='$(V)'
   AM_DEFAULT_V='$(AM_DEFAULT_VERBOSITY)'
   AC_SUBST([AM_V])
   AC_SUBST([AM_DEFAULT_V])])

dnl information on the package
AC_CONFIG_HEADERS([src/config.h])

AC_CANONICAL_HOST

OPAL_CONFIGURE_SETUP

OPAL_BASIC_SETUP

OPAL_CHECK_ATTRIBUTES
AS_IF([test "$opal_cv___attribute__deprecated" = 1],
      [SHMEM_AC_HAVE_ATTRIBUTE_DEPRECATED=1],
      [SHMEM_AC_HAVE_ATTRIBUTE_DEPRECATED=0])
AC_SUBST(SHMEM_AC_HAVE_ATTRIBUTE_DEPRECATED)

OPAL_CHECK_PMIX

AC_ARG_ENABLE([picky],
    [AS_HELP_STRING([--enable-picky],
                    [Enable developer-level compiler pickyness when building (default: disabled)])])

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [Include debugging symbols in the build (default: disabled)])])

AC_ARG_ENABLE([error-checking],
    [AS_HELP_STRING([--enable-error-checking],
                    [Enable error checking for SHMEM calls (default:disabled)])])
AS_IF([test "$enable_error_checking" = "yes"], [AC_DEFINE([ENABLE_ERROR_CHECKING], [1], [Enable argument checking])])
AM_CONDITIONAL([ENABLE_ERROR_CHECKING], [test "$enable_error_checking" = "yes"])

AC_ARG_ENABLE([deprecated-tests],
    [AS_HELP_STRING([--enable-deprecated-tests],
                    [Enable deprecated SHMEM API calls in tests (default:disabled)])])
AM_CONDITIONAL([ENABLE_DEPRECATED_TESTS], [test "$enable_deprecated_tests" = "yes"])

AC_ARG_ENABLE([hard-polling],
    [AS_HELP_STRING([--enable-hard-polling],
                    [Enable hard polling of wait calls (default:automatic)])])
AS_IF([test "$enable_hard_polling" = "yes"], [AC_DEFINE([ENABLE_HARD_POLLING], [1], [Enable hard polling])])

AC_ARG_ENABLE([completion-polling],
    [AS_HELP_STRING([--enable-completion-polling],
                    [Enable polling in quiet, fence, and local completion operations (default:disabled)])])

AC_ARG_ENABLE([hwloc-enforce-single-socket],
    [AS_HELP_STRING([--enable-hwloc-enforce-single-socket],
                    [Repin process to ensure it has affinity to cores within a single socket (default:disabled)])])
AS_IF([test "$enable_hwloc_enforce_single_socket" = "yes"], [AC_DEFINE([HWLOC_ENFORCE_SINGLE_SOCKET], [1], [Repin process to ensure it has affinity to cores within a single socket])])

AC_ARG_ENABLE([hwloc-enforce-single-numa-node],
    [AS_HELP_STRING([--enable-hwloc-enforce-single-numa-node],
                    [Repin process to ensure it has affinity to cores within a single NUMA node (default:disabled)])])
AS_IF([test "$enable_hwloc_enforce_single_numa_node" = "yes"], [AC_DEFINE([HWLOC_ENFORCE_SINGLE_NUMA_NODE], [1], [Repin process to ensure it has affinity to cores within a single NUMA node])])

AC_ARG_ENABLE([manual-progress],
    [AS_HELP_STRING([--enable-manual-progress],
                    [Enable intermittent progress calls inside transport layer (default:disabled)])])
AS_IF([test "$enable_completion_polling" = "yes" -o "$enable_manual_progress" = "yes"],
      [AC_DEFINE([DEFAULT_POLL_LIMIT], [-1], [Poll limit for local/remote completions])],
      [AC_DEFINE([DEFAULT_POLL_LIMIT], [0], [Poll limit for local/remote completions])])

AS_IF([test "$enable_manual_progress" = "yes"],
      [AC_DEFINE([ENABLE_MANUAL_PROGRESS], [1], [Enable manual progress])])

AC_ARG_ENABLE([total-data-ordering],
    [AS_HELP_STRING([--enable-total-data-ordering],
       [Configure handling of total data ordering option.  "no" or "never" to build with the assumption total data ordering will never be available.  "yes" or "always" to build with the assumption total data ordering will always be available (if not, applications will abort in start_pes()).  "check" to build with no assumptions, which may lead to a slight performance decrease on high performance networks.  (default: disabled)])])
AS_CASE([x$enable_total_data_ordering],
  [xyes|xalways], [ordering=1; msg="assume always available"],
  [xno|xnever|x], [ordering=0; msg="assume never available"],
  [xcheck], [ordering=2; msg="runtime check"],
  [AC_MSG_ERROR([Unknown ordering requirement $enable_total_data_ordering])])
AC_DEFINE_UNQUOTED([WANT_TOTAL_DATA_ORDERING], [$ordering], [0 if never check for total data ordering, 1 if always assume total data ordering, 2 to check])
AC_MSG_CHECKING([For total data ordering behavior])
AC_MSG_RESULT([$msg])

AC_ARG_ENABLE([threads],
    [AS_HELP_STRING([--disable-threads],
                    [Disable threading support (default:enabled)])])

AS_IF([test "$enable_threads" != "no"], [
       OPAL_CONFIG_POSIX_THREADS(HAVE_POSIX_THREADS=1, HAVE_POSIX_THREADS=0)
       AC_MSG_CHECKING([for working POSIX threads package])
       if test "$HAVE_POSIX_THREADS" = "1" ; then
           AC_MSG_RESULT([yes])
           AC_SUBST(PTHREAD_CFLAGS)
           AC_SUBST(PTHREAD_LIBS)
       else
           AC_MSG_RESULT([no])
       fi
       AC_DEFINE([ENABLE_THREADS], [1], [Enable threads])
       AC_CHECK_HEADERS([stdatomic.h])
       CHECK_GETTID(
           [AC_DEFINE([HAVE_SYS_GETTID], [1], [Have support for SYS_gettid syscall])],
           [AC_MSG_WARN([No support for SYS_gettid syscall])])
       AS_IF([test "$enable_thread_completion" = "yes"],
             [AC_DEFINE([USE_CTX_LOCK], [1], [Enable per-context locks])
             AC_DEFINE([USE_THREAD_COMPLETION], [1], [Support SHMEM_THREAD_MULTIPLE using FI_THREAD_COMPLETION instead of FI_THREAD_SAFE])])
       ])
AM_CONDITIONAL([HAVE_PTHREADS], [test "$HAVE_POSIX_THREADS" = "1"])

AS_IF([test "$with_hwloc" != "no"], [
        OPAL_VAR_SCOPE_PUSH([external_hwloc_happy internal_hwloc_happy  opal_hwloc_STATIC_LDFLAGS opal_hwloc_LIBS opal_hwloc_STATIC_LIBS])
        OPAL_3RDPARTY_WITH([hwloc], [hwloc], [package_hwloc])
        _OPAL_CONFIG_HWLOC_EXTERNAL(
            [external_hwloc_happy=1
             opal_hwloc_mode="external"
             AC_DEFINE([USE_HWLOC], [1], [Enable hwloc. Default behavior is to automatically enable hwloc if installation can be found])],
            [AS_IF([test -n "$with_hwloc"],
                [AC_MSG_ERROR([Could not find viable hwloc build.])],
                [external_hwloc_happy=0
                 AC_MSG_WARN([Could not find viable hwloc build.])])]
        )

        OPAL_3RDPARTY_EXTRA_DIST="$OPAL_3RDPARTY_EXTRA_DIST hwloc_tarball"
        OPAL_3RDPARTY_DISTCLEAN_DIRS="$OPAL_3RDPARTY_DISTCLEAN_DIRS hwloc_directory"

        OPAL_WRAPPER_FLAGS_ADD([LDFLAGS], [${opal_hwloc_LDFLAGS}])
        OPAL_WRAPPER_FLAGS_ADD([STATIC_LDFLAGS], [${opal_hwloc_STATIC_LDFLAGS}])
        OPAL_WRAPPER_FLAGS_ADD([LIBS], [${opal_hwloc_LIBS}])
        OPAL_WRAPPER_FLAGS_ADD([STATIC_LIBS], [${opal_hwloc_STATIC_LIBS}])
        OPAL_WRAPPER_FLAGS_ADD([PC_MODULES], [${opal_hwloc_PC_MODULES}])

        AC_CONFIG_COMMANDS_PRE([OPAL_CONFIG_HWLOC_INTERNAL_LIBS_HANDLER])

        OPAL_SUMMARY_ADD([Miscellaneous], [hwloc], [], [$opal_hwloc_mode])

        OPAL_VAR_SCOPE_POP
        ])

AS_IF([test "$enable_threads" != "no"], [
       AS_IF([test "$enable_openmp" != "no"], [
             AC_OPENMP
             AC_SUBST(AM_OPENMP_CXXFLAGS,"$OPENMP_CXXFLAGS")
             AC_SUBST(AM_OPENMP_CFLAGS,"$OPENMP_CFLAGS")])
       ])
AM_CONDITIONAL([HAVE_OPENMP], [test "$enable_threads" != "no" -a "$enable_openmp" != "no" -a "$ac_cv_prog_c_openmp" != "unsupported"])

AC_ARG_ENABLE([pthread-mutexes],
    [AS_HELP_STRING([--enable-pthread-mutexes],
                    [Use pthread mutexes instead of internal queueing locks (default:disabled)])])
AS_IF([test "$enable_pthread_mutexes" = "yes"], [AC_DEFINE([ENABLE_PTHREAD_MUTEX], [1], [Enable pthread mutexes])])

AC_ARG_ENABLE([thread-completion],
    [AS_HELP_STRING([--enable-thread-completion],
        [Support SHMEM_THREAD_MULTIPLE in OFI transport using FI_THREAD_COMPLETION thread safety model instead of FI_THREAD_SAFE (default: disabled)])])

AC_ARG_ENABLE([remote-virtual-addressing],
    [AS_HELP_STRING([--enable-remote-virtual-addressing],
                    [Enable optimizations assuming the symmetric heap and data segments are located at identical addresses at all PEs.  This may cause erroneous behavior if this assumption is violated, but will reduce the instruction count for some operations.  On Linux systems, this optimization requires disabling ASLR.  This optimization also requires that the Portals 4 implementation support BIND_INACCESSIBLE on LEs. (default: disabled)])])
AS_IF([test "$enable_remote_virtual_addressing" = "yes"],
      [AC_DEFINE([ENABLE_REMOTE_VIRTUAL_ADDRESSING], [1], [If defined, the implementation will use one LE/PT to cover all of the symmetric data and heap, setup so that no offset transformation is needed on the target virtual address.])])

AC_ARG_ENABLE([aslr-check],
    [AS_HELP_STRING([--disable-aslr-check],
                    [Disable check for address space layout randomization (ASLR).  This can be useful when ASLR is enabled, but position-independent executable generation is disabled by compiler or linker flags. (default: enabled)])])
AS_IF([test "$enable_aslr_check" = "no"],
      [AC_DEFINE(DISABLE_ASLR_CHECK_AC, [1], [Disable ASLR check])])


AC_ARG_ENABLE([lengthy-tests],
    [AS_HELP_STRING([--enable-lengthy-tests],
                    [Execute long running tests as part of "make check" (default: disabled)])])
AM_CONDITIONAL([ENABLE_LENGTHY_TESTS], [test "$enable_lengthy_tests" = "yes"])

AC_ARG_ENABLE([fortran],
    [AS_HELP_STRING([--disable-fortran],
                    [Disable building the Fortran bindings (default: enabled)])])

AC_ARG_ENABLE([cxx],
    [AS_HELP_STRING([--disable-cxx],
                    [Disable building the C++ bindings (default: enabled)])])

AC_ARG_ENABLE([c11-checks],
    [AS_HELP_STRING([--disable-c11-checks],
                    [Disable build-time type checks for C11 bindings (default: enabled)])])

AC_ARG_ENABLE([libtool-wrapper],
  [AS_HELP_STRING([--disable-libtool-wrapper],
    [Disable use of libtool executable wrappers for tests (default: enabled)])])
AS_IF([test "$enable_libtool_wrapper" = "no"], [LIBTOOL_WRAPPER_LDFLAGS="-no-install"], [LIBTOOL_WRAPPER_LDFLAGS=])
AC_SUBST([LIBTOOL_WRAPPER_LDFLAGS])

AC_ARG_ENABLE([rpath],
  [AS_HELP_STRING([--disable-rpath],
    [Disable use of rpath in compiler wrappers and building tests (default: enabled)])])
AS_IF([test "$enable_rpath" = "no"], [], [enable_wrapper_rpath="yes"])


AC_ARG_ENABLE([profiling],
    [AS_HELP_STRING([--enable-profiling],
                    [Enable profiling (pshmem) interface (default:disabled)])])
AS_IF([test "$enable_profiling" = "yes"],
      [OPAL_C_WEAK_SYMBOLS()
       AS_IF([test "$OPAL_C_HAVE_WEAK_SYMBOLS" = 1],
             [AC_DEFINE([ENABLE_PROFILING], [1], [Enable profiling (pshmem) interface])],
             [AC_MSG_ERROR("Profiling interface requires weak symbols support")])
       ])

AC_ARG_ENABLE([long-fortran-header],
    [AS_HELP_STRING([--enable-long-fortran-header],
                    [Enable long Fortran header, including all function declarations (default:disabled)])])
AS_IF([test "$enable_long_fortran_header" = "yes"], [FORTRAN_LONG_HEADER=" "], [FORTRAN_LONG_HEADER="!"])
AC_SUBST([FORTRAN_LONG_HEADER])
AM_CONDITIONAL([HAVE_LONG_FORTRAN_HEADER], [test "$enable_long_fortran_header" = "yes"])

AC_ARG_ENABLE([ofi-mr],
    [AS_HELP_STRING([--enable-ofi-mr=MODE],
                    [OFI memory registration mode: basic, scalable, or rma-event (default: scalable)])])

AS_IF([test -z "$enable_ofi_mr"], [enable_ofi_mr="scalable"])

AS_CASE([$enable_ofi_mr],
        [basic],
            [],
        [scalable],
            [AC_DEFINE([ENABLE_MR_SCALABLE], [1], [If defined, the OFI transport will use FI_MR_SCALABLE])],
        [rma?event],
            [AC_DEFINE([ENABLE_MR_RMA_EVENT], [1], [If defined, the OFI transport will use FI_MR_RMA_EVENT])
             AC_DEFINE([ENABLE_MR_SCALABLE], [1], [If defined, the OFI transport will use FI_MR_SCALABLE])],
        [AC_MSG_ERROR([Invalid OFI memory registration mode: $enable_ofi_mr])])

AC_ARG_ENABLE([mr-endpoint],
    [AS_HELP_STRING([--enable-mr-endpoint],
                    [Use FI_MR_ENDPOINT to enable cxi provider. (default: disabled)])])
AS_IF([test "$enable_mr_endpoint" = "yes"],
      [AC_DEFINE([ENABLE_MR_ENDPOINT], [1], [If defined, the OFI transport will use FI_MR_ENDPOINT])])

AC_ARG_ENABLE([ofi-manual-progress],
    [AS_HELP_STRING([--enable-ofi-manual-progress],
                    [Use FI_MANUAL_PROGRESS for data progress control mode. (default: disabled)])])
AS_IF([test "$enable_ofi_manual_progress" = "yes"],
      [AC_DEFINE([ENABLE_FI_MANUAL_PROGRESS], [1], [If defined, the OFI will use FI_MANUAL_PROGRESS as data progress mode. This is currently required for cxi provider.])])

AC_ARG_ENABLE([max-teams],
    [AS_HELP_STRING([--enable-max-teams=NUMBER],
                    [Default value for the maximum number of teams allowed (default: 10)])])

AS_IF([test -z "$enable_max_teams"],
      [AC_DEFINE([DEFAULT_TEAMS_MAX], [10], [Maximum number of teams (default)])],
      [AC_DEFINE_UNQUOTED([DEFAULT_TEAMS_MAX], [$enable_max_teams], [Maximum number of teams (custom)])])

AC_ARG_ENABLE([rpm-prefix],
    [AS_HELP_STRING([--enable-rpm-prefix],
                    [Generate RPM spec file that supports an alternate installation prefix (default:disabled)])])
AS_IF([test "$enable_rpm_prefix" = "yes"], [SOS_RPM_PREFIXED=""], [SOS_RPM_PREFIXED="# DISABLED: "])
AC_SUBST([SOS_RPM_PREFIXED])

AC_ARG_ENABLE([av-map],
    [AS_HELP_STRING([--enable-av-map],
                    [enable av-map instead of av-table in the OFI transport. (default: disabled)])])
AS_IF([test "$enable_av_map" = "yes"],
      [AC_DEFINE([USE_AV_MAP], [1], [If defined, the OFI transport will use FI_AV_MAP.  Otherwise, FI_AV_TABLE is used.])])

AC_ARG_ENABLE([bounce-buffers],
    [AS_HELP_STRING([--disable-bounce-buffers],
                    [Disable bounce buffers by default (default: enabled)])])
AS_IF([test "$enable_bounce_buffers" = "no"],
      [AC_DEFINE([DEFAULT_BOUNCE_SIZE], [0], [Default bounce buffer threshold])],
      [AC_DEFINE([DEFAULT_BOUNCE_SIZE], [2048], [Default bounce buffer threshold])])

AC_ARG_ENABLE([ofi-inject],
    [AS_HELP_STRING([--disable-ofi-inject],
                    [Disable OFI inject by default (default: enabled)])])
AS_IF([test "$enable_ofi_inject" = "no"],
      [AC_DEFINE([DISABLE_OFI_INJECT], [1], [If defined, the OFI will not use fi_inject.])])

AC_ARG_ENABLE([nonfetch-amo],
    [AS_HELP_STRING([--disable-nonfetch-amo],
                    [Disable non-fetching AMO and replace them with fetching AMO (default: enabled)])])
AS_IF([test "$enable_nonfetch_amo" = "no"],
      [AC_DEFINE([DISABLE_NONFETCH_AMO], [1], [If defined, SOS will fetch values for any AMO.])])

AC_ARG_WITH([oshrun-launcher],
    [AS_HELP_STRING([--with-oshrun-launcher],
                    [Set launcher to be used by oshrun launcher wrapper. (default: auto)])])

AC_ARG_ENABLE([memcpy],
    [AS_HELP_STRING([--enable-memcpy],
                    [Use memcpy to perform local put/get operations (default:disabled)])])

AC_ARG_ENABLE([mmap],
    [AC_HELP_STRING([--enable-mmap],
                    [Use mmap to share symmetric data and heap segments in shared memory (default:disabled)])])

AC_ARG_ENABLE([ofi-fence],
    [AS_HELP_STRING([--enable-ofi-fence],
                    [Use FI_FENCE feature to optimize put-with-signal operations. (default: disabled)])])
AS_IF([test "$enable_ofi_fence" = "yes"],
      [AC_DEFINE([USE_FI_FENCE], [1], [If defined, the OFI transport will use FI_FENCE. Otherwise, transport layer fence is used.])])

AC_ARG_ENABLE([shr-atomics],
    [AS_HELP_STRING([--enable-shr-atomics],
                    [Enable shared memory atomic operations. (default: auto)])])
AS_IF([test "$enable_shr_atomics" = "yes"],
      [AC_DEFINE([USE_SHR_ATOMICS], [1], [If defined, the shared memory layer will perform processor atomics.])])

AC_ARG_ENABLE([manpages],
    [AS_HELP_STRING([--enable-manpages],
                    [Include man pages in the installation (default:disabled)])])
AM_CONDITIONAL([ENABLE_MANPAGES], [test "$enable_manpages" = "yes"])
AS_IF([test "$enable_manpages" = "yes"], [SOS_RPM_MANPAGES=""], [SOS_RPM_MANPAGES="# DISABLED: "])
AC_SUBST([SOS_RPM_MANPAGES])

AC_ARG_ENABLE([ofi-hmem],
    [AS_HELP_STRING([--enable-ofi-hmem],
                    [Use FI_HMEM to support transfers to and from device memory. (default: disabled)])])
AS_IF([test "$enable_ofi_hmem" = "yes"],
      [AC_DEFINE([USE_FI_HMEM], [1], [If defined, the OFI transport will enable FI_HMEM.])])

PKG_INSTALLDIR()

dnl check for programs

AC_PROG_CC
AM_PROG_CC_C_O
AC_C_INLINE
AC_PROG_CXX dnl required even with --disable-cxx due to automake conditionals
m4_ifdef([AM_PROG_AR],[AM_PROG_AR])
AC_PATH_PROG([PERL],[perl])
AM_CONDITIONAL([EXTERNAL_TESTS], [false])
AM_CONDITIONAL([SHMEMX_TESTS], [true])

dnl C++ bindings require std::is_same, which is part of C++11
if test "$enable_cxx" != "no" ; then
    AX_CXX_COMPILE_STDCXX([11])
fi

# start gnu11/gnu99 check
ORIG_CFLAGS=$CFLAGS
WRAPPER_COMPILER_CC=$CC
AC_SUBST([WRAPPER_COMPILER_CC])
AC_MSG_CHECKING([if -std=gnu11 works])

AC_LANG_PUSH([C])
C_COMPILER_VERSION=$("$CC" --version 2> /dev/null)
IS_ONEAPI_COMPILER=$(echo $C_COMPILER_VERSION | $GREP -c -E 'Intel\(R\) oneAPI')

if test "$IS_ONEAPI_COMPILER" -ne "0" ; then
  CFLAGS="-std=gnu11"
else
  CFLAGS="-pedantic-errors -std=gnu11"
fi

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
                   void g(int i){}
                   #define f(X) _Generic((X), default: g)(X)
                   ]], [[
                        f(1);
                        ]]
                   )],
                   [shmem_cv_c11_works="yes"], [shmem_cv_c11_works="no"])
AS_IF([test "$shmem_cv_c11_works" = "no"],
      [AC_MSG_RESULT([no])
      CFLAGS="$ORIG_CFLAGS"
      # AC_PROG_CC_C99 is obsolete after autoconf 2.70, because
      # AC_PROG_CC should revert to C99 automatically if needed
      m4_version_prereq([2.70], [AC_PROG_CC], [AC_PROG_CC_C99])
      ],
      [AC_MSG_RESULT([yes])
      CFLAGS="-std=gnu11 $ORIG_CFLAGS"]
      )

AC_LANG_POP([C])
AM_CONDITIONAL([HAVE_C11], [test "$shmem_cv_c11_works" = "yes" -a "$enable_c11_checks" != "no"])
# end gnu11/gnu99 check


AC_PATH_PROG([M4], [m4], [false])
AS_IF([test "$M4" = "false"],
      [AC_MSG_ERROR([Could not find 'm4' macro processor])])

if test "$enable_fortran" != "no" ; then
  AC_PROG_FC
else
  FC=
fi
if test "$FC" != "" ; then
  #The oneAPI ifx compiler requires autoconf 2.70 to support AC_FC_WRAPPERS. If problems, please try ifort or updating autoconf:
  #https://community.intel.com/t5/Intel-Fortran-Compiler/ifx-2021-1-beta04-HPC-Toolkit-build-error-with-loopopt/m-p/1184181
  AC_FC_WRAPPERS
fi

AX_GCC_BUILTIN([__builtin_trap])

if test "$enable_picky" = "yes" -a "$GCC" = "yes" ; then
  CFLAGS="$CFLAGS -Wall -Wno-long-long -Wmissing-prototypes -Wstrict-prototypes -Wcomment -pedantic"
else
  CFLAGS="$CFLAGS -Wall"
fi

if test "$enable_debug" = "yes" ; then
  CFLAGS="$CFLAGS -g"
fi

AC_CACHE_SAVE

AC_SEARCH_LIBS([clock_gettime], [rt],
               [AC_DEFINE([HAVE_CLOCK_GETTIME], [1], [define if clock_gettime is available]) ])

CHECK_SCHED_GETAFFINITY(
    [AC_DEFINE([HAVE_SCHED_GETAFFINITY], [1], [define if sched_getaffinity is available])], [])

AC_SEARCH_LIBS([ceil], [m], [], AC_MSG_ERROR([unable to find ceil() function]))

dnl check for libraries

OPAL_CHECK_PORTALS4([portals4],
    [transport_portals4="yes"],
    [transport_portals4="no"])

OPAL_CHECK_OFI([ofi],
    [transport_ofi="yes"],
    [transport_ofi="no"])

AS_IF([test "$opal_ofi_happy" = "yes"],
    [transport_ofi="yes"],
    [transport_ofi="no"])

OMPI_CHECK_UCX([ucx],
    [transport_ucx="yes"],
    [transport_ucx="no"])

num_transports=""
if test -n "$with_portals4" -a "$with_portals4" != "no" ; then
    num_transports="x$num_transports"
fi
if test -n "$with_ofi" -a "$with_ofi" != "no" ; then
    num_transports="x$num_transports"
fi
if test -n "$with_ucx" -a "$with_ucx" != "no" ; then
    num_transports="x$num_transports"
fi

if test -n "$num_transports" -a "$num_transports" != "x" ; then
    AC_MSG_ERROR([Cannot select more than one transport, see --help for details])
fi

if test -n "$with_portals4" -a "$with_portals4" != "no" ; then
    transport="portals4"
    transport_ofi="no"
    transport_ucx="no"
    AC_DEFINE([USE_PORTALS4], [1], [Define if Portals 4 transport active])
elif test -n "$with_ofi" -a "$with_ofi" != "no" ; then
    transport="ofi"
    transport_portals="no"
    transport_ucx="no"
    AC_DEFINE([USE_OFI], [1], [Define if OFI transport active])
elif test -n "$with_ucx" -a "$with_ucx" != "no" ; then
    transport="ucx"
    transport_ofi="no"
    transport_portals="no"
    AC_DEFINE([USE_UCX], [1], [Define if UCX transport active])
    AC_DEFINE([ENABLE_HARD_POLLING], [1], [Enable hard polling])
else
    transport="none"
    transport_portals4="no"
    transport_ofi="no"
    transport_ucx="no"
    AC_MSG_WARN([No network transport requested])
fi

AM_CONDITIONAL([USE_PORTALS4], [test "$transport_portals4" = "yes"])
AM_CONDITIONAL([USE_OFI], [test "$transport_ofi" = "yes"])
AM_CONDITIONAL([USE_UCX], [test "$transport_ucx" = "yes"])

SANDIA_CHECK_XPMEM(
    [transport_xpmem="yes"],
    [transport_xpmem="no"])

CHECK_CMA(
    [transport_cma="yes"],
    [transport_cma="no"])


num_shared_transports=0
if test -n "$with_xpmem" -a "$with_xpmem" != "no" ; then
    ((num_shared_transports+=1))
fi
if test -n "$with_cma" -a "$with_cma" != "no" ; then
    ((num_shared_transports+=1))
fi
if test -n "$enable_mmap" -a "$enable_mmap" != "no" ; then
    ((num_shared_transports+=1))
fi

# If more than one shared memory transport requested, user needs to choose one:
if [[ $num_shared_transports -gt 1 ]] ; then
    echo "Selected" $num_shared_transports "shared memory transports, please choose one."
    AC_MSG_ERROR([Only one shared memory transport is allowed (mmap, XPMEM, or CMA), see --help for details])
elif test -n "$enable_mmap" -a "$enable_mmap" != "no" ; then
    transport_mmap="yes"
    transport_cma="no"
    transport_xpmem="no"
    AC_DEFINE([USE_MMAP], [1], [Define if mmap transport is active])
    AC_CHECK_LIB(rt, shm_open, [MMAP_LDFLAGS="-lrt"])
    AC_SUBST([MMAP_LDFLAGS])
elif test -n "$with_xpmem" -a "$with_xpmem" != "no" ; then
    transport_mmap="no"
    transport_cma="no"
    AC_DEFINE([USE_XPMEM], [1], [Define if XPMEM transport is active])
elif test -n "$with_cma" -a "$with_cma" != "no" ; then
    transport_mmap="no"
    transport_xpmem="no"
    AC_DEFINE([USE_CMA], [1], [Define if Cross Memory Attach transport is active])
    AC_DEFINE([_GNU_SOURCE], [1], [CMA transport header requires global definition of _GNU_SOURCE])
else
    transport_mmap="no"
    transport_xpmem="no"
    transport_cma="no"
    AC_MSG_RESULT([Shared memory transport not requested])
fi

if test "$enable_memcpy" = "yes" -a "$transport_xpmem" = "no" -a "$transport_cma" = "no" -a "$transport_mmap" = "no" ; then
    transport_memcpy="yes"
    AC_DEFINE([USE_MEMCPY], [1], [Define to use memcpy for local put/get communication])
elif test "$transport_xpmem" = "yes" -o "$transport_cma" = "yes" -o "$transport_mmap" = "yes" ; then
    transport_memcpy="yes"
else
    transport_memcpy="no"
fi

AM_CONDITIONAL([USE_XPMEM], [test "$transport_xpmem" = "yes"])
AM_CONDITIONAL([USE_CMA], [test "$transport_cma" = "yes"])
AM_CONDITIONAL([USE_MMAP], [test "$transport_mmap" = "yes"])

AS_IF([test "$transport_xpmem" = "yes" -o "$transport_cma" = "yes" -o "$transport_mmap" = "yes"],
      [AC_DEFINE([USE_ON_NODE_COMMS], [1], [Define if any on-node comm transport is available])
       AC_DEFINE([ENABLE_HARD_POLLING], [1], [Enable hard polling])
      ])

if test "$enable_shr_atomics" = "yes"; then
    transport_shr_atomics="yes"
else
    transport_shr_atomics="no"
fi

if [[[ "$enable_shr_atomics" != "no" && ( "$transport_xpmem" = "yes" || "$transport_mmap" = "yes" ) && "$transport" = "none" ]]]; then
    transport_shr_atomics="yes"
    AC_DEFINE([USE_SHR_ATOMICS], [1], [If defined, the shared memory layer will perform processor atomics.])
fi

AC_ARG_ENABLE([pmi-simple], [AS_HELP_STRING([--enable-pmi-simple],
              [Use MPICH simple PMI-1 library for process management])])
AC_ARG_ENABLE([pmi-mpi], [AS_HELP_STRING([--enable-pmi-mpi],
              [Use MPI for process management])])

AS_IF([test "$enable_pmi_simple" = "yes" -o "$enable_pmi_mpi" = "yes"],
     [TEST_RUNNER='$(abs_top_builddir)/src/oshrun -n $(NPROCS)'],
     [OPAL_CHECK_PMI(pmi_type)])

AS_IF([test "$opal_external_pmix_version_found" = 1],
     [pmi_CPPFLAGS="$opal_external_pmix_CPPFLAGS"
      pmi_LDFLAGS="$opal_external_pmix_LDFLAGS"
      pmi_LIBS="$opal_external_pmix_LIBS"])

AS_IF([test "$enable_pmi_simple" = "yes" -a "$enable_pmi_mpi" = "yes"],
      [AC_MSG_ERROR([Cannot enable both simple PMI and MPI-PMI])])

AS_IF([test "$enable_pmi_mpi" = "yes"],
      [AC_DEFINE([ENABLE_PMI_MPI], [1], [Enable argument checking])])

AM_CONDITIONAL([USE_PMI_MPI], [test "$enable_pmi_mpi" = "yes"])
AM_CONDITIONAL([USE_PMI_SIMPLE], [test "$enable_pmi_simple" = "yes"])
AM_CONDITIONAL([USE_PMI1], [test "$pmi_type" = "pmi1"])
AM_CONDITIONAL([USE_PMI2], [test "$pmi_type" = "pmi2"])
AM_CONDITIONAL([USE_PMIX], [test "$opal_external_pmix_version_found" = 1])

# Backtrace
## execinfo
AC_CHECK_HEADERS(execinfo.h)
bt_execinfo="no"
if test "$ac_cv_header_execinfo_h" = "yes" ; then
  AC_SEARCH_LIBS(backtrace, execinfo)
  AC_CHECK_FUNCS(backtrace backtrace_symbols)
  if test "$ac_cv_func_backtrace" = "yes" && test "$ac_cv_func_backtrace_symbols" = "yes"; then
    bt_execinfo="yes"
  fi
fi

## gdb
AC_PATH_PROGS(GDB_PATH, $GDB gdb, , /usr/bin:${PATH})
if test -n "$GDB_PATH"; then
  AC_DEFINE_UNQUOTED(GDB_PATH, "$GDB_PATH", [Path to GNU debugger])
fi

if test "$bt_execinfo" = "yes"; then
  use_bt_option="$use_bt_option execinfo"
  AC_DEFINE([USE_BT_EXECINFO], [1], [Execinfo library calls for backtrace])
  if test "$IS_ONEAPI_COMPILER" -eq "0" ; then
  case $host_os in
      linux*)
           CFLAGS="$CFLAGS -rdynamic"
           ;;
   esac
  fi
fi
if test -n "$GDB_PATH"; then
  use_bt_option="$use_bt_option gdb"
  AC_DEFINE([USE_BT_GDB], [1], [gdb for backtrace])
fi
if test -z "$use_bt_option"; then
  use_bt_option=" not available"
fi

dnl check for header files
AC_CHECK_HEADERS([fnmatch.h])
AS_IF([test "$enable_pmi_simple" = "yes"],
      [AC_CHECK_HEADERS([assert.h arpa/inet.h sys/types.h unistd.h stdlib.h string.h strings.h])
      AC_DEFINE([USE_PMI_PORT], [1], [Use port])])

dnl check for types

AC_LANG_PUSH([C])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ float _Complex v; ]] )],
                  [],
                  [AC_MSG_ERROR(C Compiler does not support _Complex)])
AC_LANG_POP([C])

SHMEM_FIND_INT_TYPE([char], [$transport])
SHMEM_FIND_INT_TYPE([signed char], [$transport])
SHMEM_FIND_INT_TYPE([short], [$transport])
SHMEM_FIND_INT_TYPE([int], [$transport])
SHMEM_FIND_INT_TYPE([long], [$transport])
SHMEM_FIND_INT_TYPE([long long], [$transport])
SHMEM_FIND_INT_TYPE([ptrdiff_t], [$transport])

SHMEM_FIND_UINT_TYPE([unsigned char], [$transport])
SHMEM_FIND_UINT_TYPE([unsigned short], [$transport])
SHMEM_FIND_UINT_TYPE([unsigned int], [$transport])
SHMEM_FIND_UINT_TYPE([unsigned long], [$transport])
SHMEM_FIND_UINT_TYPE([unsigned long long], [$transport])
SHMEM_FIND_UINT_TYPE([size_t], [$transport])

AC_CHECK_SIZEOF([float])
AC_CHECK_SIZEOF([double])
AC_CHECK_SIZEOF([long double])
AC_CHECK_SIZEOF([void*])

dnl Determine psync and pwrk sizes for collectives

C_LOG_MAXPES=32
AC_MSG_CHECKING([size of SHMEM_BCAST_SYNC_SIZE])
    C_BCAST_SYNC_SIZE=1
    AC_SUBST(C_BCAST_SYNC_SIZE)
AC_MSG_RESULT([$C_BCAST_SYNC_SIZE])
AC_MSG_CHECKING([size of SHMEM_REDUCE_SYNC_SIZE])
    C_REDUCE_SYNC_SIZE=`$PERL -e "print $C_BCAST_SYNC_SIZE + 2 + $C_LOG_MAXPES"`
    AC_SUBST(C_REDUCE_SYNC_SIZE)
AC_MSG_RESULT([$C_REDUCE_SYNC_SIZE])
AC_MSG_CHECKING([size of SHMEM_BARRIER_SYNC_SIZE])
    C_BARRIER_SYNC_SIZE=`$PERL -e "print $ac_cv_sizeof_int * 8 * ($ac_cv_sizeof_int / $ac_cv_sizeof_long)"`
    AC_SUBST(C_BARRIER_SYNC_SIZE)
AC_MSG_RESULT([$C_BARRIER_SYNC_SIZE])
AC_MSG_CHECKING([size of SHMEM_COLLECT_SYNC_SIZE])
    linear_sync_size=`$PERL -e "print 2 + $C_BARRIER_SYNC_SIZE"`
    tree_sync_size=`$PERL -e "print $C_BCAST_SYNC_SIZE + 3"`
    recdbl_sync_size=`$PERL -e "print $ac_cv_sizeof_int * 8 * ($ac_cv_sizeof_int / $ac_cv_sizeof_long)"`
    C_COLLECT_SYNC_SIZE=`$PERL -e "use List::Util qw(max); print max($linear_sync_size, $tree_sync_size, $recdbl_sync_size)"`
    AC_SUBST(C_COLLECT_SYNC_SIZE)
AC_MSG_RESULT([$C_COLLECT_SYNC_SIZE])
AC_MSG_CHECKING([size of SHMEM_ALLTOALL_SYNC_SIZE])
    C_ALLTOALL_SYNC_SIZE=$C_BARRIER_SYNC_SIZE
    AC_SUBST(C_ALLTOALL_SYNC_SIZE)
AC_MSG_RESULT([$C_ALLTOALL_SYNC_SIZE])
AC_MSG_CHECKING([size of SHMEM_ALLTOALLS_SYNC_SIZE])
    C_ALLTOALLS_SYNC_SIZE=$C_BARRIER_SYNC_SIZE
    AC_SUBST(C_ALLTOALLS_SYNC_SIZE)
AC_MSG_RESULT([$C_ALLTOALLS_SYNC_SIZE])
AC_MSG_CHECKING([size of SHMEM_SYNC_SIZE])
    C_SYNC_SIZE=`$PERL -e "use List::Util qw (max); print max((       \
                                              $C_BCAST_SYNC_SIZE,     \
                                              $C_REDUCE_SYNC_SIZE,    \
                                              $C_BARRIER_SYNC_SIZE,   \
                                              $C_COLLECT_SYNC_SIZE,   \
                                              $C_ALLTOALL_SYNC_SIZE,  \
                                              $C_ALLTOALLS_SYNC_SIZE));"`
    AC_SUBST(C_SYNC_SIZE)
AC_MSG_RESULT([$C_SYNC_SIZE])
AC_MSG_CHECKING([size of SHMEM_REDUCE_MIN_WRKDATA_SIZE])
    C_REDUCE_MIN_WRKDATA_SIZE=1
    AC_SUBST(C_REDUCE_MIN_WRKDATA_SIZE)
AC_MSG_RESULT([$C_REDUCE_MIN_WRKDATA_SIZE])

AC_MSG_CHECKING([if long can hold lock_t])
AC_RUN_IFELSE([AC_LANG_SOURCE([[struct lock_s { int a; int b; }; int main(void) { if (sizeof(long) == sizeof(struct lock_s)) return 0; else return 1; }]])],[AC_MSG_RESULT([yes])],[
                AC_MSG_RESULT([no])
                AC_MSG_ERROR([Lock API requires long can hold 'struct { int a; int b; }']) ],[])

if test "$FC" != "" ; then
  AC_DEFINE([ENABLE_FORTRAN],[1],[Fortran support enabled])
  AC_MSG_CHECKING([if complex sizes are appropriate])
  AC_RUN_IFELSE([AC_LANG_SOURCE([[float _Complex x; double _Complex y; int main(void) { if (sizeof(x)==8 && sizeof(y)==16) return 0; else return 1; }]])],[AC_MSG_RESULT([yes])],[
                  AC_MSG_RESULT([no])
                  AC_MSG_ERROR([float _Complex and double _Complex must be 4 and 8 bytes, respectively]) ],[])
  # make sure Fortran supports ISO_C_BINDING (Fortran 2003)
  AC_CACHE_CHECK([if Fortran compiler supports ISO_C_BINDING],
    [shmem_cv_fortran_iso_c_binding],
    [AC_LANG_PUSH([Fortran])
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM(, [[
        use, intrinsic :: ISO_C_BINDING
	integer(c_int) :: i
	i = 5
]])], [shmem_cv_fortran_iso_c_binding="yes"],
      [shmem_cv_fortran_iso_c_binding="no"])
      AC_LANG_POP([Fortran])])
  AS_IF([test "$shmem_cv_fortran_iso_c_binding" = "no"],
    [AC_MSG_ERROR([Fortran compiler does not support ISO_C_BINDING, which is required for Sandia OpenSHMEM.])])

  # Get size of Fortran types
  OMPI_FORTRAN_GET_SIZEOF([], [LOGICAL])
  OMPI_FORTRAN_GET_SIZEOF([], [CHARACTER])
  OMPI_FORTRAN_GET_SIZEOF([], [INTEGER])
  OMPI_FORTRAN_GET_SIZEOF([], [REAL])
  OMPI_FORTRAN_GET_SIZEOF([], [DOUBLE PRECISION])
  OMPI_FORTRAN_GET_SIZEOF([], [COMPLEX])
  OMPI_FORTRAN_GET_SIZEOF([], [DOUBLE COMPLEX])

  # find transport integer type corresponding to INTEGER
  SHMEM_FIND_FORTRAN_INT_TYPE([INTEGER], [$transport])

  # find C type corresponding to given Fortran types
  OMPI_FIND_TYPE([INTEGER], [char,     short, int, long, long long])
  OMPI_FIND_TYPE([DOUBLE PRECISION], [float, double, long double])

  # Find kind values for 4 & 8 byte integer types in Fortran (used in
  # shmem.fh)
  OMPI_FORTRAN_GET_KIND_VALUE([C_INT32_T], [FORTRAN_INT4_KIND])
  OMPI_FORTRAN_GET_KIND_VALUE([C_INT64_T], [FORTRAN_INT8_KIND])
  OMPI_FORTRAN_GET_KIND_VALUE([C_FLOAT], [FORTRAN_REAL4_KIND])
  OMPI_FORTRAN_GET_KIND_VALUE([C_DOUBLE], [FORTRAN_REAL8_KIND])
  OMPI_FORTRAN_GET_KIND_VALUE([C_FLOAT_COMPLEX], [FORTRAN_COMP4_KIND])
  OMPI_FORTRAN_GET_KIND_VALUE([C_DOUBLE_COMPLEX], [FORTRAN_COMP8_KIND])

  # The collectives are all implemented in C and require the psync to
  # hold a variable number of C longs.  Find how many INTEGERs it takes to
  # create a memory region as big as the required C region.
  FORTRAN_BCAST_SYNC_SIZE=`$PERL -e "use POSIX; print ceil($ac_cv_sizeof_long / $ompi_cv_fortran_sizeof_INTEGER * $C_BCAST_SYNC_SIZE)"`
  FORTRAN_REDUCE_SYNC_SIZE=`$PERL -e "use POSIX; print ceil($ac_cv_sizeof_long / $ompi_cv_fortran_sizeof_INTEGER * $C_REDUCE_SYNC_SIZE)"`
  FORTRAN_BARRIER_SYNC_SIZE=`$PERL -e "use POSIX; print ceil($ac_cv_sizeof_long / $ompi_cv_fortran_sizeof_INTEGER * $C_BARRIER_SYNC_SIZE)"`
  FORTRAN_COLLECT_SYNC_SIZE=`$PERL -e "use POSIX; print ceil($ac_cv_sizeof_long / $ompi_cv_fortran_sizeof_INTEGER * $C_COLLECT_SYNC_SIZE)"`
  FORTRAN_ALLTOALL_SYNC_SIZE=`$PERL -e "use POSIX; print ceil($ac_cv_sizeof_long / $ompi_cv_fortran_sizeof_INTEGER * $C_ALLTOALL_SYNC_SIZE)"`
  FORTRAN_ALLTOALLS_SYNC_SIZE=`$PERL -e "use POSIX; print ceil($ac_cv_sizeof_long / $ompi_cv_fortran_sizeof_INTEGER * $C_ALLTOALLS_SYNC_SIZE)"`
  FORTRAN_REDUCE_MIN_WRKDATA_SIZE=`$PERL -e "use POSIX; print ceil($ac_cv_sizeof_long / $ompi_cv_fortran_sizeof_INTEGER * $C_REDUCE_MIN_WRKDATA_SIZE)"`
  FORTRAN_SYNC_SIZE=`$PERL -e "use List::Util qw (max); print max((             \
                                                  $FORTRAN_BCAST_SYNC_SIZE,     \
                                                  $FORTRAN_REDUCE_SYNC_SIZE,    \
                                                  $FORTRAN_BARRIER_SYNC_SIZE,   \
                                                  $FORTRAN_COLLECT_SYNC_SIZE,   \
                                                  $FORTRAN_ALLTOALL_SYNC_SIZE,  \
                                                  $FORTRAN_ALLTOALLS_SYNC_SIZE));"`
else
  FORTRAN_BCAST_SYNC_SIZE=0
  FORTRAN_REDUCE_SYNC_SIZE=0
  FORTRAN_BARRIER_SYNC_SIZE=0
  FORTRAN_COLLECT_SYNC_SIZE=0
  FORTRAN_ALLTOALL_SYNC_SIZE=0
  FORTRAN_ALLTOALLS_SYNC_SIZE=0
  FORTRAN_SYNC_SIZE=0
  FORTRAN_REDUCE_MIN_WRKDATA_SIZE=0
  FORTRAN_INT4_KIND=0
  FORTRAN_INT8_KIND=0
  FORTRAN_REAL4_KIND=0
  FORTRAN_REAL8_KIND=0
  FORTRAN_COMP4_KIND=0
  FORTRAN_COMP8_KIND=0
  AC_DEFINE([DTYPE_FORTRAN_INTEGER],[-1],[Fortran integer C type (disabled)])
fi
if test "$ac_cv_sizeof_voidp" = "4" ; then
  FORTRAN_PTR_KIND=$FORTRAN_INT4_KIND
else
  FORTRAN_PTR_KIND=$FORTRAN_INT8_KIND
fi
AC_SUBST(FORTRAN_BCAST_SYNC_SIZE)
AC_SUBST(FORTRAN_REDUCE_SYNC_SIZE)
AC_SUBST(FORTRAN_BARRIER_SYNC_SIZE)
AC_SUBST(FORTRAN_COLLECT_SYNC_SIZE)
AC_SUBST(FORTRAN_ALLTOALL_SYNC_SIZE)
AC_SUBST(FORTRAN_ALLTOALLS_SYNC_SIZE)
AC_SUBST(FORTRAN_SYNC_SIZE)
AC_SUBST(FORTRAN_REDUCE_MIN_WRKDATA_SIZE)
AC_SUBST(FORTRAN_INT4_KIND)
AC_SUBST(FORTRAN_INT8_KIND)
AC_SUBST(FORTRAN_REAL4_KIND)
AC_SUBST(FORTRAN_REAL8_KIND)
AC_SUBST(FORTRAN_COMP4_KIND)
AC_SUBST(FORTRAN_COMP8_KIND)
AC_SUBST(FORTRAN_PTR_KIND)

AM_CONDITIONAL([HAVE_FORTRAN], [test "$FC" != ""])

AM_CONDITIONAL([HAVE_CXX], [test "$enable_cxx" != "no" ])

AM_CONDITIONAL([ENABLE_PROFILING], [test "$enable_profiling" = "yes" ])

dnl check for structures

dnl check for compiler characteristics

# make all symbols hidden by default
OPAL_CHECK_VISIBILITY
AS_IF([test "$opal_visibility_define" = 1],
      [SHMEM_AC_HAVE_ATTRIBUTE_VISIBILITY=1],
      [SHMEM_AC_HAVE_ATTRIBUTE_VISIBILITY=0])
AC_SUBST(SHMEM_AC_HAVE_ATTRIBUTE_VISIBILITY)
CFLAGS="$CFLAGS $OPAL_VISIBILITY_CFLAGS"

# Disable ASLR at link time by instructing the linker not to generate
# position-independent executable code
aslr_LDFLAGS=
case $host_os in
    darwin*)
           aslr_LDFLAGS="-Wl,-no_pie"
           ;;
esac
AC_SUBST(aslr_LDFLAGS)

dnl check for library functions

AC_MSG_CHECKING([for data segment pointer])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#ifdef __APPLE__
#include <mach-o/getsect.h>
#else
#pragma weak __data_start
#pragma weak _end
extern int __data_start;
extern int _end;
#endif

/* Ensure data segment is not empty.  Anything simpler gets optimized
 * away by the compiler. */
int ensure_nonempty_data(void) {
    static int ncall = 0;
    return ncall++;
}

int main(void) {
    void *base;
    long length;
#ifdef __APPLE__
    base = (void*) get_etext();
    length = get_end() - get_etext();
#else
    if (&__data_start == (int*) 0 || &_end == (int*) 0) return 1;
    base = (void*) &__data_start;
    length = (long) ((char*) &_end - (char*) &__data_start);
#endif
    if (!(base != (void*)0 && length > 0)) return 2;
    return 0;
}]])],[AC_MSG_RESULT(found)],[ AC_MSG_RESULT([not found])
      AC_MSG_ERROR([Could not locate data segment])
    ],[])

dnl final output

LT_INIT

OPAL_CASE_SENSITIVE_FS_SETUP

DISTCHECK_CONFIGURE_FLAGS=
if test -n "$with_portals4" -a "$with_portals4" != "no" -a "$with_portals4" != "yes" ; then
  DISTCHECK_CONFIGURE_FLAGS="$DISTCHECK_CONFIGURE_FLAGS --with-portals4=${with_portals4}"
fi
if test -n "$with_ofi" -a "$with_ofi" != "no" -a "$with_ofi" != "yes" ; then
  DISTCHECK_CONFIGURE_FLAGS="$DISTCHECK_CONFIGURE_FLAGS --with-ofi=${with_ofi}"
fi
if test -n "$with_xpmem" -a "$with_xpmem" != "no" -a "$with_xpmem" != "yes" ; then
  DISTCHECK_CONFIGURE_FLAGS="$DISTCHECK_CONFIGURE_FLAGS --with-xpmem=${with_xpmem}"
fi
if test -n "$enable_mmap" -a "$enable_mmap" != "no" -a "$enable_mmap" != "yes" ; then
  DISTCHECK_CONFIGURE_FLAGS="$DISTCHECK_CONFIGURE_FLAGS --enable-mmap=${enable_mmap}"
fi
if test -n "$with_pmi" -a "$with_pmi" != "no" -a "$with_pmi" != "yes" ; then
  DISTCHECK_CONFIGURE_FLAGS="$DISTCHECK_CONFIGURE_FLAGS --with-pmi=${with_pmi}"
fi
if test -n "$with_hwloc" -a "$with_hwloc" != "no" -a "$with_hwloc" != "yes" ; then
  DISTCHECK_CONFIGURE_FLAGS="$DISTCHECK_CONFIGURE_FLAGS --with-hwloc=${with_hwloc}"
fi
if test "$enable_pmi_simple" = "yes" ; then
  DISTCHECK_CONFIGURE_FLAGS="$DISTCHECK_CONFIGURE_FLAGS --enable-pmi-simple"
fi
if test -n "$FCFLAGS" ; then
  DISTCHECK_CONFIGURE_FLAGS="$DISTCHECK_CONFIGURE_FLAGS FCFLAGS=\"$FCFLAGS\""
fi
AC_SUBST(DISTCHECK_CONFIGURE_FLAGS)

dnl Cause Automake to delete emacs turds
CLEANFILES="*~"
AC_SUBST(CLEANFILES)

# configuration for --with-oshrun-launcher
WRAPPER_SEARCH_LAUNCHERS="yod mpirun mpiexec srun"
if test -n "$with_oshrun_launcher" ; then
    WRAPPER_SEARCH_LAUNCHERS="$with_oshrun_launcher"
fi
AC_SUBST(WRAPPER_SEARCH_LAUNCHERS)

# last minute updates of the flags so that tests don't fail oddly if
# portals libdir isn't in LD_LIBRARY_PATH...
if test "$transport_portals4" = "yes" ; then
    CPPFLAGS="$CPPFLAGS $portals4_CPPFLAGS"
    LDFLAGS="$LDFLAGS $portals4_LDFLAGS"
    LIBS="$LIBS $portals4_LIBS"
    WRAPPER_COMPILER_EXTRA_LDFLAGS="$portals4_LDFLAGS"
    WRAPPER_COMPILER_EXTRA_LIBS="$portals4_LIBS"
fi

if test "$transport_ofi" = "yes" ; then
    CPPFLAGS="$CPPFLAGS $ofi_CPPFLAGS"
    LDFLAGS="$LDFLAGS $ofi_LDFLAGS"
    LIBS="$LIBS $ofi_LIBS"
    WRAPPER_COMPILER_EXTRA_LDFLAGS="$ofi_LDFLAGS"
    WRAPPER_COMPILER_EXTRA_LIBS="$ofi_LIBS"
fi

if test "$transport_ucx" = "yes" ; then
    CPPFLAGS="$CPPFLAGS $ucx_CPPFLAGS"
    LDFLAGS="$LDFLAGS $ucx_LDFLAGS"
    LIBS="$LIBS $ucx_LIBS"
    WRAPPER_COMPILER_EXTRA_LDFLAGS="$ucx_LDFLAGS"
    WRAPPER_COMPILER_EXTRA_LIBS="$ucx_LIBS"
fi

if test "$transport_xpmem" = "yes" ; then
    CPPFLAGS="$CPPFLAGS $XPMEM_CPPFLAGS"
    LDFLAGS="$LDFLAGS $XPMEM_LDFLAGS"
    LIBS="$LIBS $XPMEM_LIBS"
    WRAPPER_COMPILER_EXTRA_LDFLAGS="$XPMEM_LDFLAGS"
    WRAPPER_COMPILER_EXTRA_LIBS="$XPMEM_LIBS"
fi

if test -n "opal_hwloc_LDFLAGS"; then
    WRAPPER_COMPILER_EXTRA_LDFLAGS="$WRAPPER_COMPILER_EXTRA_LDFLAGS $opal_hwloc_LDFLAGS"
fi
if test "$transport_mmap" = "yes" ; then
    CPPFLAGS="$CPPFLAGS $MMAP_CPPFLAGS"
    LDFLAGS="$LDFLAGS $MMAP_LDFLAGS"
    LIBS="$LIBS $MMAP_LIBS"
    WRAPPER_COMPILER_EXTRA_LDFLAGS="$MMAP_LDFLAGS"
    WRAPPER_COMPILER_EXTRA_LIBS="$MMAP_LIBS"
fi

CPPFLAGS="$CPPFLAGS $pmi_CPPFLAGS"
LDFLAGS="$LDFLAGS $pmi_LDFLAGS $aslr_LDFLAGS"
LIBS="$LIBS $pmi_LIBS"
WRAPPER_COMPILER_EXTRA_LDFLAGS="$WRAPPER_COMPILER_EXTRA_LDFLAGS $pmi_LDFLAGS $aslr_LDFLAGS"
WRAPPER_COMPILER_EXTRA_LIBS="$WRAPPER_COMPILER_LIBS $pmi_LIBS"

# Need the libtool binary before setting up rpath
LT_OUTPUT

OPAL_TOP_BUILDDIR="`pwd`"
OPAL_SETUP_RPATH

AS_IF([test "$enable_pmi_simple" = "yes"],
     [WRAPPER_COMPILER_EXTRA_LIBS="$WRAPPER_COMPILER_EXTRA_LIBS -lpmi_simple"])

AC_MSG_CHECKING([for OPAL LDFLAGS])
RPATHIFY_LDFLAGS([WRAPPER_COMPILER_EXTRA_LDFLAGS])
AS_IF([test "$enable_wrapper_rpath" = "yes"],
     [WRAPPER_COMPILER_EXTRA_LDFLAGS="$WRAPPER_COMPILER_EXTRA_LDFLAGS -Wl,-rpath -Wl,${libdir}"])
AC_MSG_RESULT([$WRAPPER_COMPILER_EXTRA_LDFLAGS])
AC_SUBST(WRAPPER_COMPILER_EXTRA_LDFLAGS)
AC_SUBST(WRAPPER_COMPILER_EXTRA_LIBS)

AC_CONFIG_FILES([Makefile
  sandia-openshmem.spec
  man/Makefile
  bindings/Makefile
  pmi-simple/Makefile
  mpp/Makefile
  mpp/shmem.fh
  mpp/shmemx.fh
  mpp/shmem-def.h
  src/Makefile
  src/sandia-openshmem.pc
  modules/Makefile
  modules/tests-sos/Makefile
  modules/tests-sos/test/Makefile
  modules/tests-sos/test/unit/Makefile
  modules/tests-sos/test/shmemx/Makefile
  modules/tests-sos/test/spec-example/Makefile
  modules/tests-sos/test/include/Makefile
  modules/tests-sos/test/performance/Makefile
  modules/tests-sos/test/performance/shmem_perf_suite/Makefile
  modules/tests-sos/test/performance/tests/Makefile
  modules/tests-sos/test/apps/Makefile])

AC_OUTPUT

AS_IF([test "$enable_pmi_mpi" != "yes" -a "$enable_pmi_simple" != "yes" -a "$opal_external_pmix_version_found" != 1 -a "$ompi_check_pmi_happy" != "yes" -a -z "$pmi_type"],
         [AC_MSG_ERROR([No PMI client interface was configured, consider --enable-pmi-simple or --with-pmi])])

AS_IF([test -z "$num_transports"],
      [AC_MSG_WARN([No network transport found, library will be unable to exchange remote messages])])

AS_IF([test "$shmem_cv_c11_works" != "yes"],
      [AC_MSG_WARN([C compiler does not support _Generic, unable to verify and test C11 bindings])])

AS_IF([test "$enable_c11_checks" == "no"],
      [AC_MSG_WARN([C11 _Generic type checks disabled, not verifying type coverage of C11 bindings])])

AS_IF([test "$enable_cxx" == "no"],
      [AC_MSG_WARN([No C++ compiler or C++ disabled, unable to verify and test C++ bindings])])

AS_IF([test "$enable_fortran" == "no"],
      [AC_MSG_WARN([No Fortran compiler or Fortran disabled, unable to test Fortran bindings])])

AS_IF([test "$enable_fortran" != "no"],
      [AC_MSG_WARN([Fortran bindings have been deprecated as of OpenSHMEM v1.4])])

FORT="$FC"
if test "$FORT" = "" ; then
  FORT="none"
fi

if test "$enable_cxx" == "no"; then
  CXX="none"
fi

echo ""
echo "Compilers:"
echo "  C:              $CC"
echo "  C++:            $CXX"
echo "  Fortran:        $FORT"
echo ""
echo "Transports:"
echo "  Portals 4:      $transport_portals4"
echo "  OFI:            $transport_ofi"
echo "  UCX:            $transport_ucx"
echo ""
echo "On Node Communication:"
echo "  XPMEM:          $transport_xpmem"
echo "  CMA:            $transport_cma"
echo "  mmap:           $transport_mmap"
echo "  memcpy (self):  $transport_memcpy"
echo "  Shr. atomics:   $transport_shr_atomics"
echo ""
echo "Global Options:"
if test "$enable_remote_virtual_addressing" = "yes"; then
    echo "  Remote VA:      yes"
else
    echo "  Remote VA:      no"
fi
if test "$enable_threads" != "no"; then
    echo "  Thread support: yes"
else
    echo "  Thread support: no"
fi
if test "$opal_hwloc_mode" = "external"; then
    echo "  hwloc support:  yes"
else
    echo "  hwloc support:  no"
fi
if test "$enable_error_checking" != "yes"; then
    echo "  Error checking: no"
else
    echo "  Error checking: yes"
fi
echo ""

if test "$transport_ofi" = "yes"; then
    echo "OFI Transport:"
    echo "  Memory reg.:    $enable_ofi_mr"

    if test "$enable_av_map" = "yes"; then
        echo "  Addr. vector:   map"
    else
        echo "  Addr. vector:   table"
    fi

    echo ""
fi

echo "Backtrace:       $use_bt_option"
echo ""
