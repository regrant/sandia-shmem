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


#define FC_START_PES FC_FUNC_(start_pes, START_PES)
void SHMEM_FUNCTION_ATTRIBUTES FC_START_PES(fortran_integer_t *npes);
void
FC_START_PES(fortran_integer_t *npes)
{
    shmem_internal_start_pes(*npes);
}


#define FC_SHMEM_INIT FC_FUNC_(shmem_init, SHMEM_INIT)
void SHMEM_FUNCTION_ATTRIBUTES FC_SHMEM_INIT(void);
void
FC_SHMEM_INIT(void)
{
    int tl_provided;
    shmem_internal_init(SHMEM_THREAD_SINGLE, &tl_provided);
}


#define FC_SHMEM_GLOBAL_EXIT FC_FUNC_(shmem_global_exit, SHMEM_GLOBAL_EXIT)
void SHMEM_FUNCTION_ATTRIBUTES FC_SHMEM_GLOBAL_EXIT(fortran_integer_t *status);
void
FC_SHMEM_GLOBAL_EXIT(fortran_integer_t *status)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    shmem_internal_global_exit(*status);
}


#define FC_SHMEM_FINALIZE FC_FUNC_(shmem_finalize, SHMEM_FINALIZE)
void SHMEM_FUNCTION_ATTRIBUTES FC_SHMEM_FINALIZE(void);
void
FC_SHMEM_FINALIZE(void)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    shmem_internal_finalize();
}


#define FC_SHMEM_INFO_GET_VERSION FC_FUNC_(shmem_info_get_version, SHMEM_INFO_GET_VERSION)
void SHMEM_FUNCTION_ATTRIBUTES FC_SHMEM_INFO_GET_VERSION(fortran_integer_t *major, fortran_integer_t *minor);
void
FC_SHMEM_INFO_GET_VERSION(fortran_integer_t *major, fortran_integer_t *minor)
{
    SHMEM_ERR_CHECK_INITIALIZED();

    *major = SHMEM_MAJOR_VERSION;
    *minor = SHMEM_MINOR_VERSION;
}


#define FC_SHMEM_INFO_GET_NAME FC_FUNC_(shmem_info_get_name, SHMEM_INFO_GET_NAME)
void SHMEM_FUNCTION_ATTRIBUTES FC_SHMEM_INFO_GET_NAME(char *name, int32_t name_len);
void
FC_SHMEM_INFO_GET_NAME(char *name, int32_t name_len)
{
    size_t len = sizeof(SHMEM_VENDOR_STRING) - 1; /* Length without NULL terminator */

    SHMEM_ERR_CHECK_INITIALIZED();

    strncpy(name, SHMEM_VENDOR_STRING, SHMEM_MAX_NAME_LEN);

    /* Drop NULL terminator and fill with whitespace */
    memset(&name[len], ' ', SHMEM_MAX_NAME_LEN - len);
}
