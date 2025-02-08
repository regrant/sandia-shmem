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

#include <string.h>
#include "config.h"

#define SHMEM_INTERNAL_INCLUDE
#include "shmem.h"
#include "shmem_internal.h"
#include "shmemx.h"

#ifdef ENABLE_PROFILING
#include "pshmem.h"

#pragma weak start_pes = pstart_pes
#define start_pes pstart_pes

#pragma weak shmem_init = pshmem_init
#define shmem_init pshmem_init

#pragma weak shmemx_heap_preinit = pshmemx_heap_preinit
#define shmemx_heap_preinit pshmemx_heap_preinit

#pragma weak shmemx_heap_postinit = pshmemx_heap_postinit
#define shmemx_heap_postinit pshmemx_heap_postinit

#pragma weak shmem_init_thread = pshmem_init_thread
#define shmem_init_thread pshmem_init_thread

#pragma weak shmemx_heap_preinit_thread = pshmemx_heap_preinit_thread
#define shmemx_heap_preinit_thread pshmemx_heap_preinit_thread

#pragma weak shmem_finalize = pshmem_finalize
#define shmem_finalize pshmem_finalize

#pragma weak shmem_info_get_version = pshmem_info_get_version
#define shmem_info_get_version pshmem_info_get_version

#pragma weak shmem_info_get_name = pshmem_info_get_name
#define shmem_info_get_name pshmem_info_get_name

#pragma weak shmem_query_thread = pshmem_query_thread
#define shmem_query_thread pshmem_query_thread

#pragma weak shmem_global_exit = pshmem_global_exit
#define shmem_global_exit pshmem_global_exit

#endif /* ENABLE_PROFILING */

void SHMEM_FUNCTION_ATTRIBUTES
start_pes(int npes)
{
    if (!shmem_internal_init_counter) {
        shmem_internal_start_pes(npes);
    }
}


void SHMEM_FUNCTION_ATTRIBUTES
shmem_init(void)
{
    int tl_provided, ret;

    ret = shmem_internal_init(SHMEM_THREAD_SINGLE, &tl_provided);
    if (ret) abort();
}


void SHMEM_FUNCTION_ATTRIBUTES
shmemx_heap_preinit(void)
{
    int tl_provided, ret;

    if (shmem_internal_init_counter) {
        RAISE_ERROR_STR("attempt to reinitialize library");
    }

    ret = shmem_internal_heap_preinit(SHMEM_THREAD_SINGLE, &tl_provided);
    if (ret) abort();
}


void SHMEM_FUNCTION_ATTRIBUTES
shmemx_heap_postinit(void)
{
    int ret = shmem_internal_heap_postinit();
    if (ret) abort();
}


int SHMEM_FUNCTION_ATTRIBUTES
shmem_init_thread(int tl_requested, int *tl_provided)
{
    int ret;

    ret = shmem_internal_init(tl_requested, tl_provided);
    return ret;
}


int SHMEM_FUNCTION_ATTRIBUTES
shmemx_heap_preinit_thread(int tl_requested, int *tl_provided)
{
    int ret;
    if (shmem_internal_init_counter) {
        RAISE_ERROR_STR("attempt to reinitialize library");
    }

    ret = shmem_internal_heap_preinit(tl_requested, tl_provided);
    return ret;
}


void SHMEM_FUNCTION_ATTRIBUTES
shmem_query_thread(int *provided)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    *provided = shmem_internal_thread_level;
}

void SHMEM_FUNCTION_ATTRIBUTES
shmem_query_initialized(int *initialized)
{
    *initialized = shmem_internal_init_counter;
}

void SHMEM_FUNCTION_ATTRIBUTES
shmem_global_exit(int status)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    shmem_internal_global_exit(status);
}


void SHMEM_FUNCTION_ATTRIBUTES
shmem_finalize(void)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    shmem_internal_finalize();
}


void SHMEM_FUNCTION_ATTRIBUTES
shmem_info_get_version(int *major, int *minor)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    *major = SHMEM_MAJOR_VERSION;
    *minor = SHMEM_MINOR_VERSION;
}


void SHMEM_FUNCTION_ATTRIBUTES
shmem_info_get_name(char *name)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    strncpy(name, SHMEM_VENDOR_STRING, SHMEM_MAX_NAME_LEN);
    name[SHMEM_MAX_NAME_LEN-1] = '\0'; /* Ensure string is null terminated */
}

