# -*- Makefile -*-
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

AM_CPPFLAGS = -I$(top_builddir)/mpp -I$(top_srcdir)/mpp

GEN_BINDINGS = \
	atomic_c.c \
	atomic_nbi_c.c \
	collectives_c.c \
	data_c.c \
	synchronization_c.c \
	teams_c.c


if HAVE_FORTRAN
GEN_BINDINGS += \
	collectives_f.c \
	data_f.c
endif

lib_LTLIBRARIES = libsma.la
libsma_la_SOURCES = \
	shmem_free_list.h \
	shmem_free_list.c \
	shmem_atomic.h \
	runtime.h \
	runtime_util.c \
	shmem_internal.h \
	shmem_internal_op.h \
	shmem_comm.h \
	shmem_collectives.h \
	shmem_synchronization.h \
	shmem_accessibility.h \
	shmem_remote_pointer.h \
	shmem_lock.h \
	malloc.c \
	init.c \
	collectives.c \
	init_c.c \
	query_c.c \
	accessibility_c.c \
	symmetric_heap_c.c \
	remote_pointer_c.c \
	lock_c.c \
	cache_management_c.c \
	transport.h \
	util.c \
	uthash.h \
	shmem_decl.h \
	shmem_env.c \
	shmem_env.h \
	shmem_env_defs.h \
	contexts_c.c \
	contexts.c \
	sessions_c.c \
	perf_counters_c.c \
	backtrace.c \
	shmem_team.c \
	shmem_team.h

BUILT_SOURCES = $(GEN_BINDINGS) \
		build_info.h \
		shr_transport.h

nodist_libsma_la_SOURCES = $(BUILT_SOURCES)

CLEANFILES += $(BUILT_SOURCES)

if HAVE_FORTRAN
libsma_la_SOURCES += \
	init_f.c \
	query_f.c \
	accessibility_f.c \
	symmetric_heap_f.c \
	remote_pointer_f.c \
	atomic_f.c \
	synchronization_f.c \
	lock_f.c \
	cache_management_f.c
endif

if USE_PORTALS4
libsma_la_SOURCES += \
	transport_portals4.h \
	transport_portals4.c
endif

if USE_OFI
libsma_la_SOURCES += \
	transport_ofi.h \
	transport_ofi.c
endif

if USE_UCX
libsma_la_SOURCES += \
	transport_ucx.h \
	transport_ucx.c
endif

if !USE_PORTALS4
if !USE_OFI
if !USE_UCX
libsma_la_SOURCES += \
	transport_none.c \
	transport_none.h
endif
endif
endif

if USE_XPMEM
libsma_la_SOURCES += \
	transport_xpmem.h \
	transport_xpmem.c
endif

if USE_CMA
libsma_la_SOURCES += \
	transport_cma.h \
	transport_cma.c
endif

if USE_PMI_SIMPLE
AM_CPPFLAGS += -I$(top_srcdir)/pmi-simple
AM_LDFLAGS = -L$(top_builddir)/pmi-simple -lpmi_simple
libsma_la_SOURCES += \
	runtime-pmi.c
else
if USE_PMI_MPI
libsma_la_SOURCES += \
	runtime-mpi.c
else
if USE_PMI1
libsma_la_SOURCES += \
	runtime-pmi.c
else
if USE_PMI2
libsma_la_SOURCES += \
	runtime-pmi2.c
else
if USE_PMIX
libsma_la_SOURCES += \
	runtime-pmix.c
endif USE_PMIX
endif !USE_PMI2
endif !USE_PMI1
endif !USE_PMI_MPI
endif !USE_PMI_SIMPLE

pkgconfig_DATA = sandia-openshmem.pc

bin_SCRIPTS = oshcc oshrun
CLEANFILES += oshcc oshrun
if CASE_SENSITIVE_FS
bin_SCRIPTS += oshCC
CLEANFILES += oshCC
endif
if HAVE_CXX
bin_SCRIPTS += oshc++
CLEANFILES += oshc++
endif
if HAVE_FORTRAN
bin_SCRIPTS += oshfort
CLEANFILES += oshfort
endif

EXTRA_DIST = shmem_compiler_script.in \
             shmem_launcher_script.in \
             atomic_c.c4              \
             atomic_nbi_c.c4          \
             collectives_c.c4         \
             collectives_f.c4         \
             data_c.c4                \
             data_f.c4                \
             synchronization_c.c4     \
             teams_c.c4               \
             shr_transport.h4

do_subst = sed -e 's|[@]PERL[@]|$(PERL)|g' \
               -e 's|[@]WRAPPER_COMPILER_CC[@]|$(WRAPPER_COMPILER_CC)|g' \
               -e 's|[@]CXX[@]|$(CXX)|g' \
               -e 's|[@]FC[@]|$(FC)|g' \
               -e 's|[@]WRAPPER_COMPILER_INCLUDEDIR[@]|$(includedir)|g' \
               -e 's|[@]WRAPPER_COMPILER_LIBDIR[@]|$(libdir)|g' \
               -e 's|[@]WRAPPER_COMPILER_EXTRA_LDFLAGS[@]|$(WRAPPER_COMPILER_EXTRA_LDFLAGS)|g' \
               -e 's|[@]WRAPPER_COMPILER_EXTRA_LIBS[@]|$(WRAPPER_COMPILER_EXTRA_LIBS)|g'

do_subst_launcher = sed -e 's|[@]WRAPPER_SEARCH_LAUNCHERS[@]|$(WRAPPER_SEARCH_LAUNCHERS)|g' 

AM_V_SED = $(am__v_SED_@AM_V@)
am__v_SED_ = $(am__v_SED_@AM_DEFAULT_V@)
am__v_SED_0 = @echo "  SED     " $@;

build_info.h: $(top_builddir)/config.status
	$(AM_V_GEN) echo "#ifndef BUILD_INFO_H" > $@
	@echo "#define BUILD_INFO_H" >> $@
	@if [ -f $(top_srcdir)/.git/HEAD ]; then \
	     echo "#define SOS_GIT_VERSION \"$(shell git describe --abbrev=8 --dirty --always --tags) ($(shell git rev-parse --abbrev-ref HEAD))\"" >> $@; \
	fi;
	@echo "#define SOS_CONFIGURE_ARGS \"$(shell $(top_builddir)/config.status --config)\"" >> $@;
	@echo "#define SOS_BUILD_DATE \"$(shell date)\"" >> $@;
	@echo "#define SOS_BUILD_CC \"$(CC)\"" >> $@;
	@echo "#define SOS_BUILD_CFLAGS \"$(CFLAGS)\"" >> $@;
	@echo "#endif" >> $@

oshcc: shmem_compiler_script.in Makefile
	$(AM_V_SED)$(do_subst) -e 's|[@]LANG[@]|C|g' < $(srcdir)/shmem_compiler_script.in > oshcc
	@chmod +x oshcc

oshc++: shmem_compiler_script.in Makefile
	$(AM_V_SED)$(do_subst) -e 's|[@]LANG[@]|C++|g' < $(srcdir)/shmem_compiler_script.in > oshc++
	@chmod +x oshc++

oshCC: shmem_compiler_script.in Makefile
	$(AM_V_SED)$(do_subst) -e 's|[@]LANG[@]|C++|g' < $(srcdir)/shmem_compiler_script.in > oshCC
	@chmod +x oshCC

oshfort: shmem_compiler_script.in Makefile
	$(AM_V_SED)$(do_subst) -e 's|[@]LANG[@]|Fortran|g' < $(srcdir)/shmem_compiler_script.in > oshfort
	@chmod +x oshfort

oshrun: shmem_launcher_script.in Makefile
	$(AM_V_SED)$(do_subst_launcher) < $(srcdir)/shmem_launcher_script.in > oshrun
	@chmod +x oshrun

SUFFIXES = .c4 .h4

.c4.c: $(top_srcdir)/bindings/shmem_bind_c.m4 $(top_srcdir)/bindings/shmem_bind_f.m4
	$(AM_V_GEN) $(M4) -I$(top_srcdir)/bindings -I$(top_builddir)/bindings $< > $@

.h4.h: $(top_srcdir)/bindings/shmem_bind_c.m4 $(top_srcdir)/bindings/shmem_bind_f.m4
	$(AM_V_GEN) $(M4) -I$(top_srcdir)/bindings -I$(top_builddir)/bindings $< > $@
