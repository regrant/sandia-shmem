/* -*- C -*-
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S.  Government
 * retains certain rights in this software.
 *
 * Copyright (c) 2017 Intel Corporation. All rights reserved.
 * This software is available to you under the BSD license.
 *
 * This file is part of the Sandia OpenSHMEM software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 *
 */

#include "config.h"

#ifdef HAVE_SCHED_GETAFFINITY
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#endif

#include <stdlib.h>
#include <sys/time.h>
#include <sys/param.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#define SHMEM_INTERNAL_INCLUDE
#include "shmem.h"
#include "shmem_internal.h"
#include "shmem_collectives.h"
#include "shmem_comm.h"
#include "runtime.h"
#include "build_info.h"
#include "shmem_team.h"

#if defined(ENABLE_REMOTE_VIRTUAL_ADDRESSING) && defined(__linux__)
#include <sys/personality.h>
#endif

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_vm.h>
#else
/* Declare data_start and end as weak to avoid a linker error if the symbols
 * are not present.  During initialization we check if the symbols exist. */
#pragma weak __data_start
#pragma weak _end
extern int __data_start;
extern int _end;
#endif

#ifdef __APPLE__
static vm_address_t get_base_address(mach_port_t task, size_t *region_size, void *addr)
{
    kern_return_t kret;
    vm_region_basic_info_data_t info;
    vm_size_t size;
    mach_port_t object_name;
    mach_msg_type_number_t count;
    mach_vm_address_t address = (addr ? (mach_vm_address_t)addr : 0);
    count = VM_REGION_BASIC_INFO_COUNT_64;

    kret = mach_vm_region(task, &address, (mach_vm_size_t *)&size,
                          VM_REGION_BASIC_INFO, (vm_region_info_t)&info,
                          &count, &object_name);
    shmem_internal_assertp(kret == KERN_SUCCESS);

    *region_size = (size_t) size;
    return address;
}
#endif

void *shmem_internal_heap_base = NULL;
long shmem_internal_heap_length = 0;
void *shmem_internal_data_base = NULL;
long shmem_internal_data_length = 0;

void *shmem_external_heap_base = NULL;
long shmem_external_heap_length = 0;

int shmem_external_heap_pre_initialized = 0;
int shmem_external_heap_device_type = -1;
int shmem_external_heap_device = -1;

int shmem_internal_my_pe = -1;
int shmem_internal_num_pes = -1;
int shmem_internal_init_counter = 0;
int shmem_internal_finalized = 0;
int shmem_internal_initialized_with_start_pes = 0;
int shmem_internal_global_exit_called = 0;

int shmem_internal_thread_level;

unsigned int shmem_internal_rand_seed;

#ifdef USE_HWLOC
#include <hwloc.h>
hwloc_topology_t shmem_internal_topology;
#endif

#ifdef ENABLE_THREADS
shmem_internal_mutex_t shmem_internal_mutex_alloc;
shmem_internal_mutex_t shmem_internal_mutex_rand_r;
#endif

static char *shmem_internal_thread_level_str[4] = { "SINGLE", "FUNNELED",
                                                    "SERIALIZED", "MULTIPLE" };

static void
shmem_internal_randr_init(void)
{
    shmem_internal_rand_seed = shmem_internal_my_pe;

#ifdef ENABLE_THREADS
    SHMEM_MUTEX_INIT(shmem_internal_mutex_rand_r);
#endif

    return;
}

static void
shmem_internal_randr_fini(void)
{

#ifdef ENABLE_THREADS
    SHMEM_MUTEX_DESTROY(shmem_internal_mutex_rand_r);
#endif

    return;
}


static void
shmem_internal_shutdown(void)
{
    if (!shmem_internal_init_counter ||
        shmem_internal_finalized) {
        return;
    }

    shmem_internal_barrier_all();

    shmem_internal_init_counter -= 1;

    if (shmem_internal_init_counter > 0) {
        return;
    }

    shmem_internal_finalized = 1;

    shmem_internal_team_fini();

    shmem_transport_fini();

    shmem_shr_transport_fini();

    SHMEM_MUTEX_DESTROY(shmem_internal_mutex_alloc);

    shmem_internal_randr_fini();

    shmem_internal_symmetric_fini();
    shmem_runtime_fini();
}


static void
shmem_internal_shutdown_atexit(void)
{
    if ( shmem_internal_init_counter && !shmem_internal_finalized &&
         !shmem_internal_initialized_with_start_pes && !shmem_internal_global_exit_called &&
         shmem_internal_my_pe == 0) {
        RAISE_WARN_STR("shutting down without a call to shmem_finalize");
    }
    shmem_internal_init_counter = 1;
    shmem_internal_shutdown();
}


void
shmem_internal_start_pes(int npes)
{
    int ret, tl_provided;

    shmem_internal_initialized_with_start_pes = 1;
    ret = shmem_internal_init(SHMEM_THREAD_SINGLE, &tl_provided);

    if (ret) abort();
}


int
shmem_internal_heap_preinit(int tl_requested, int *tl_provided)
{
    int ret;

    int runtime_initialized   = 0;
    int enable_node_ranks     = 0;

    /* Parse environment variables into shmem_internal_params */
    ret = shmem_internal_parse_env();
    if (ret) return ret;

    /* set up threading */
    SHMEM_MUTEX_INIT(shmem_internal_mutex_alloc);
#ifdef ENABLE_THREADS
    shmem_internal_thread_level = tl_requested;
    *tl_provided = tl_requested;
#else
    shmem_internal_thread_level = SHMEM_THREAD_SINGLE;
    *tl_provided = SHMEM_THREAD_SINGLE;
#endif

#if USE_ON_NODE_COMMS
    enable_node_ranks = 1;
#elif USE_OFI
    enable_node_ranks = (shmem_internal_params.OFI_STX_AUTO) ? 1 : 0;
#endif

    if (!shmem_internal_params.TEAM_SHARED_ONLY_SELF)
        enable_node_ranks = 1;

    ret = shmem_runtime_init(enable_node_ranks);
    if (0 != ret) {
        fprintf(stderr, "ERROR: runtime init failed: %d\n", ret);
        goto cleanup_runtime;
    }
    runtime_initialized = 1;
    shmem_internal_my_pe = shmem_runtime_get_rank();
    shmem_internal_num_pes = shmem_runtime_get_size();

    /* Ensure that the vendor string will not cause an overflow in user code */
    if (sizeof(SHMEM_VENDOR_STRING) > SHMEM_MAX_NAME_LEN) {
        RETURN_ERROR_MSG("SHMEM_VENDOR_STRING length (%zu) exceeds SHMEM_MAX_NAME_LEN (%d)\n",
                         sizeof(SHMEM_VENDOR_STRING), SHMEM_MAX_NAME_LEN);
        goto cleanup_runtime;
    }

    /* Unless the user asked for it, disable bounce buffering in MULTIPLE
     * because of threading overheads */
    if (shmem_internal_thread_level == SHMEM_THREAD_MULTIPLE &&
        !shmem_internal_params.BOUNCE_SIZE_provided) {
        shmem_internal_params.BOUNCE_SIZE = 0;
    }

    /* Print library parameters */
    if (0 == shmem_internal_my_pe) {
        if (shmem_internal_params.VERSION || shmem_internal_params.INFO ||
            shmem_internal_params.DEBUG)
        {
            printf(PACKAGE_STRING "\n");
        }

        if (shmem_internal_params.INFO) {
            shmem_internal_print_env();
            printf("\n");
        }

        if (shmem_internal_params.DEBUG) {
            char *wrapped_configure_args = shmem_util_wrap(SOS_CONFIGURE_ARGS, 60,
                                                           "                        ");
            char *wrapped_build_cflags   = shmem_util_wrap(SOS_BUILD_CFLAGS, 60,
                                                           "                        ");

            printf("Build information:\n");
#ifdef SOS_GIT_VERSION
            printf("%-23s %s\n", "  Git Version", SOS_GIT_VERSION);
#endif
            printf("%-23s %s\n", "  Configure Args", wrapped_configure_args);
            printf("%-23s %s\n", "  Build Date", SOS_BUILD_DATE);
            printf("%-23s %s\n", "  Build CC", SOS_BUILD_CC);
            printf("%-23s %s\n", "  Build CFLAGS", wrapped_build_cflags);
            printf("\n");

            free(wrapped_configure_args);
            free(wrapped_build_cflags);
        }

        fflush(NULL);
    }

    /* ASLR is an OS security feature that randomizes the address map of each
     * process.  Remote virtual addressing assumes that symmetric addresses are
     * identical across processes.  ASLR can violate this assumption.
     *
     * However, ASLR does not always preclude identical symmetric addresses
     * across PEs.  Linking the application with -no-pie can cause the OS to
     * load the data segment at symmetric addresses.  The heap is mmap'd
     * relative to the location of the data segment and will also be symmetric.
     * Thus, we allow advanced users to disable this check. */
#if defined(ENABLE_REMOTE_VIRTUAL_ADDRESSING) && defined(__linux__) && !defined(DISABLE_ASLR_CHECK_AC)
    if (!shmem_internal_params.DISABLE_ASLR_CHECK) {
        FILE *aslr = fopen("/proc/sys/kernel/randomize_va_space", "r");
        if (aslr) {
            int aslr_status = fgetc(aslr);
            if (aslr_status != EOF && aslr_status != '0') {
                int persona = personality(0xffffffff);
                /* Check if ASLR was disabled with setarch */
                if (! (persona & ADDR_NO_RANDOMIZE)) {
                    RAISE_ERROR_MSG("Remote virtual addressing is enabled; however, address space layout randomization\n"
                                    RAISE_PE_PREFIX
                                    "is present.  Disable ASLR or rebuild without '--enable-remote-virtual-addressing'.\n"
                                    RAISE_PE_PREFIX
                                    "This error message can be disabled by setting SHMEM_DISABLE_ASLR_CHECK or building\n"
                                    RAISE_PE_PREFIX
                                    "with --disable-aslr-check.\n",
                                    shmem_internal_my_pe, shmem_internal_my_pe, shmem_internal_my_pe);
                }
            }
            fclose(aslr);
        }
    }
#endif /* ENABLE_REMOTE_VIRTUAL_ADDRESSING */


    /* Find symmetric data */
#ifdef __APPLE__
    size_t region_size;
    static int in_bss;

    /* Start of this memory region (at address 0) */
    vm_address_t addr = get_base_address(mach_task_self(), &region_size, 0);
    shmem_internal_data_base = (void*)addr;

    /* End of the BSS memory region (using an unititialized static variable) */
    addr = get_base_address(mach_task_self(), &region_size, &in_bss);
    shmem_internal_data_length = ((char*)addr + region_size) - (char*)shmem_internal_data_base;
#else
    /* We declare data_start and end as weak symbols, which allows them to
     * remain unbound after dynamic linking.  This is needed for compatibility
     * with binaries (e.g. forked processes or tools) that are used with
     * OpenSHMEM programs but don't themselves use OpenSHMEM.  Such binaries
     * need not be compiled with the OpenSHMEM library and, as a result, will
     * not have exposed these symbols for dynamic linking.  However, if the
     * OpenSHMEM library has a strong dependence on the symbols, the dynamic
     * linker will flag an error when loading the binary.
     *
     * If the data_start and end symbols are unbound, the dynamic linker will
     * assign them an lvalue of 0.  Here, we check that the binary that
     * initializes OpenSHMEM has exposed these symbols, enabling the library to
     * locate its symmetric data segment. */

    if (&__data_start == (int*) 0 || &_end == (int*) 0)
        RETURN_ERROR_MSG("Unable to locate symmetric data segment (%p, %p)\n",
                         (void*) &__data_start, (void*) &_end);

    shmem_internal_data_base = (void*) &__data_start;
    shmem_internal_data_length = (long) ((char*) &_end - (char*) &__data_start);
#endif

    return 0;

 cleanup_runtime:
    if (runtime_initialized) {
        shmem_runtime_fini();
    }
    abort();
}

int
shmem_internal_heap_postinit(void)
{
    int ret;

    int transport_initialized = 0;
    int shr_initialized       = 0;
    int randr_initialized     = 0;
    int teams_initialized     = 0;
    int enable_node_ranks     = 0;

    /* create symmetric heap */
    ret = shmem_internal_symmetric_init();
    if (0 != ret) {
        RETURN_ERROR_MSG("Symmetric heap initialization failed (%d)\n", ret);
        goto cleanup_postinit;
    }

    DEBUG_MSG("Thread level=%s, Num. PEs=%d\n"
              RAISE_PE_PREFIX
              "Sym. heap=%p len=%ld -- data=%p len=%ld\n",
              shmem_internal_thread_level_str[shmem_internal_thread_level],
              shmem_internal_num_pes,
              shmem_internal_my_pe,
              shmem_internal_heap_base, shmem_internal_heap_length,
              shmem_internal_data_base, shmem_internal_data_length);

#ifdef HAVE_SCHED_GETAFFINITY
#ifdef USE_HWLOC
    ret = hwloc_topology_init(&shmem_internal_topology);
    SHMEM_CHECK_GOTO_MSG(ret != 0, hwloc_exit, "hwloc_topology_init failed (%s). Please verify your hwloc installation\n", strerror(errno));

    ret = hwloc_topology_set_io_types_filter(shmem_internal_topology, HWLOC_TYPE_FILTER_KEEP_ALL);
    SHMEM_CHECK_GOTO_MSG(ret != 0, hwloc_exit, "hwloc_topology_set_io_types_filter failed (%s). Please verify your hwloc installation\n", strerror(errno));

    ret = hwloc_topology_load(shmem_internal_topology);
    SHMEM_CHECK_GOTO_MSG(ret != 0, hwloc_exit, "hwloc_topology_load failed (%s). Please verify your hwloc installation\n", strerror(errno));
#if defined(HWLOC_ENFORCE_SINGLE_SOCKET) || defined(HWLOC_ENFORCE_SINGLE_NUMA_NODE)
    hwloc_bitmap_t bindset = hwloc_bitmap_alloc();
    hwloc_bitmap_t bindset_all = hwloc_bitmap_alloc();
    hwloc_bitmap_t bindset_covering_obj = hwloc_bitmap_alloc();

    ret = hwloc_get_proc_last_cpu_location(shmem_internal_topology, getpid(), bindset, HWLOC_CPUBIND_PROCESS);
    SHMEM_CHECK_GOTO_MSG(ret != 0, hwloc_cleanup, "hwloc_get_proc_last_cpu_location failed (%s). Please verify your hwloc installation\n", strerror(errno));

    ret = hwloc_get_proc_cpubind(shmem_internal_topology, getpid(), bindset_all, HWLOC_CPUBIND_PROCESS);
    SHMEM_CHECK_GOTO_MSG(ret != 0, hwloc_cleanup, "hwloc_get_proc_cpubind failed (%s). Please verify your hwloc installation\n", strerror(errno));
#ifdef HWLOC_ENFORCE_SINGLE_SOCKET
    hwloc_obj_t covering_obj = hwloc_get_next_obj_covering_cpuset_by_type(shmem_internal_topology, bindset, HWLOC_OBJ_PACKAGE, NULL);
    SHMEM_CHECK_GOTO_MSG(!covering_obj, hwloc_cleanup,
                         "hwloc_get_next_obj_covering_cpuset_by_type failed (could not detect object of type 'HWLOC_OBJ_PACKAGE' in provided cpuset). Please verify your hwloc installation\n");
#else /* HWLOC_ENFORCE_SINGLE_NUMA_NODE */
    hwloc_obj_t covering_obj = hwloc_get_next_obj_covering_cpuset_by_type(shmem_internal_topology, bindset, HWLOC_OBJ_NUMANODE, NULL);
    SHMEM_CHECK_GOTO_MSG(!covering_obj, hwloc_cleanup,
                         "hwloc_get_next_obj_covering_cpuset_by_type failed (could not detect object of type 'HWLOC_OBJ_NUMANODE' in provided cpuset). Please verify your hwloc installation\n");
#endif
    hwloc_bitmap_and(bindset_covering_obj, bindset_all, covering_obj->cpuset);
    ret = hwloc_set_proc_cpubind(shmem_internal_topology, getpid(), bindset_covering_obj, HWLOC_CPUBIND_PROCESS); /* Include HWLOC_CPUBIND_STRICT in flags? */
    SHMEM_CHECK_GOTO_MSG(ret != 0, hwloc_cleanup, "hwloc_set_proc_cpubind failed (%s). Please verify your hwloc installation\n", strerror(errno));

    hwloc_cleanup:
        hwloc_bitmap_free(bindset);
        hwloc_bitmap_free(bindset_all);
        hwloc_bitmap_free(bindset_covering_obj);
#endif // HWLOC_ENFORCE_SINGLE_SOCKET || HWLOC_ENFORCE_SINGLE_NUMA_NODE
    hwloc_exit:
#endif // USE_HWLOC

    if (shmem_internal_params.DEBUG) {
        cpu_set_t my_set;

        CPU_ZERO(&my_set);

        ret = sched_getaffinity(0, sizeof(my_set), &my_set);

        if (ret == 0) {
            char cores_str[SHMEM_INTERNAL_DIAG_STRLEN];
            char *cores_str_wrap;
            int core_count = 0;

            for (int i = 0; i < CPU_SETSIZE; i++) {
                if (CPU_ISSET(i, &my_set))
                    core_count++;
            }

            size_t off = snprintf(cores_str, sizeof(cores_str),
                                  "Affinity to %d processor cores: { ", core_count);

            for (int i = 0; i < CPU_SETSIZE; i++) {
                if (CPU_ISSET(i, &my_set)) {
                    off += snprintf(cores_str+off, sizeof(cores_str)-off, "%d ", i);
                    if (off >= sizeof(cores_str)) break;
                }
            }
            if (off < sizeof(cores_str)-1)
                off += snprintf(cores_str+off, sizeof(cores_str)-off, "}");

            cores_str_wrap = shmem_util_wrap(cores_str, SHMEM_INTERNAL_DIAG_WRAPLEN,
                                             RAISE_PREFIX);
            DEBUG_MSG("%s\n", cores_str_wrap);
            free(cores_str_wrap);
        }
    }
#endif // HAVE_SCHED_GETAFFINITY

    /* Initialize transport devices */
    ret = shmem_transport_init();
    if (0 != ret) {
        RETURN_ERROR_MSG("Transport init failed (%d)\n", ret);
        goto cleanup_postinit;
    }

    ret = shmem_shr_transport_init();
    if (0 != ret) {
        RETURN_ERROR_MSG("Shared memory transport init failed (%d)\n", ret);
        goto cleanup_postinit;
    }

    /* exchange information */
    ret = shmem_runtime_exchange();
    if (0 != ret) {
        RETURN_ERROR_MSG("Runtime exchange failed (%d)\n", ret);
        goto cleanup_postinit;
    }

    DEBUG_MSG("Local rank=%d, Num. local=%d, Shr. rank=%d, Num. shr=%d\n",
              enable_node_ranks ? shmem_runtime_get_node_rank(shmem_internal_my_pe) : 0,
              enable_node_ranks ? shmem_runtime_get_node_size() : 1,
              shmem_internal_get_shr_rank(shmem_internal_my_pe),
              shmem_internal_get_shr_size());

    /* finish transport initialization after information sharing. */
    ret = shmem_transport_startup();
    if (0 != ret) {
        RETURN_ERROR_MSG("Transport startup failed (%d)\n", ret);
        goto cleanup_postinit;
    }
    transport_initialized = 1;

    ret = shmem_shr_transport_startup();
    if (0 != ret) {
        RETURN_ERROR_MSG("Shared memory transport startup failed (%d)\n", ret);
        goto cleanup_postinit;
    }
    shr_initialized = 1;

    ret = shmem_internal_collectives_init();
    if (ret != 0) {
        RETURN_ERROR_MSG("Initialization of collectives failed (%d)\n", ret);
        goto cleanup_postinit;
    }

    ret = shmem_internal_team_init();
    if (ret != 0) {
        RETURN_ERROR_MSG("Initialization of teams failed (%d)\n", ret);
        goto cleanup_postinit;
    }
    teams_initialized = 1;

    shmem_internal_randr_init();
    randr_initialized = 1;

    atexit(shmem_internal_shutdown_atexit);

    /* finish up */
#ifndef USE_PMIX
    shmem_runtime_barrier();
#endif
    return 0;

 cleanup_postinit:
    if (transport_initialized) {
        shmem_transport_fini();
    }

    if (shr_initialized) {
        shmem_shr_transport_fini();
    }

    if (randr_initialized) {
        shmem_internal_randr_fini();
    }

    if (teams_initialized) {
        shmem_internal_team_fini();
    }

    if (NULL != shmem_internal_data_base) {
        shmem_internal_symmetric_fini();
    }
    abort();
}

int
shmem_internal_init(int tl_requested, int *tl_provided)
{
    int ret;

    if (shmem_internal_init_counter == 0) {
        ret = shmem_internal_heap_preinit(tl_requested, tl_provided);
        if (ret) goto cleanup;

        ret = shmem_internal_heap_postinit();
        if (ret) goto cleanup;
    }

    shmem_internal_init_counter += 1;
    return 0;

 cleanup:
    abort();
}

void shmem_internal_finalize(void)
{
    shmem_internal_shutdown();
}


void
shmem_internal_global_exit(int status)
{
    char str[256];

    snprintf(str, 256, "PE %d called shmem_global_exit with status %d", shmem_internal_my_pe, status);

    shmem_internal_global_exit_called = 1;
    shmem_runtime_abort(status, str);
}

