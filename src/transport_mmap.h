/* -*- C -*-
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S.  Government
 * retains certain rights in this software.
 *
 * Copyright (c) 2023 Intel Corporation. All rights reserved.
 * This software is available to you under the BSD license.
 *
 * This file is part of the Sandia OpenSHMEM software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
 *
 */

#ifndef TRANSPORT_MMAP_H
#define TRANSPORT_MMAP_H

#include <string.h>
#include <inttypes.h>

struct shmem_transport_mmap_peer_info_t {
    void *data_attach_ptr;
    void *heap_attach_ptr;
    void *data_ptr;
    void *heap_ptr;
};

extern struct shmem_transport_mmap_peer_info_t *shmem_transport_mmap_peers;

#ifdef ENABLE_ERROR_CHECKING
#define MMAP_GET_REMOTE_ACCESS(target, rank, ptr)                       \
    do {                                                                \
        if (((void*) target > shmem_internal_data_base) &&              \
            ((char*) target < (char*) shmem_internal_data_base + shmem_internal_data_length)) { \
            ptr = (char*) target - (char*) shmem_internal_data_base +   \
                (char*) shmem_transport_mmap_peers[rank].data_ptr;      \
        } else if (((void*) target >= shmem_internal_heap_base) &&      \
                   ((char*) target < (char*) shmem_internal_heap_base + shmem_internal_heap_length)) { \
            ptr = (char*) target - (char*) shmem_internal_heap_base +   \
                (char*) shmem_transport_mmap_peers[rank].heap_ptr;      \
        } else {                                                        \
            ptr = NULL;                                                 \
        }                                                               \
    } while (0)
#else
#define MMAP_GET_REMOTE_ACCESS(target, rank, ptr)                       \
    do {                                                                \
        if ((void*) target < shmem_internal_heap_base) {                \
            ptr = (char*) target - (char*) shmem_internal_data_base +   \
                (char*) shmem_transport_mmap_peers[rank].data_ptr;      \
        } else {                                                        \
            ptr = (char*) target - (char*) shmem_internal_heap_base +   \
                (char*) shmem_transport_mmap_peers[rank].heap_ptr;      \
        }                                                               \
    } while (0)
#endif

int shmem_transport_mmap_init(void);

int shmem_transport_mmap_startup(void);

int shmem_transport_mmap_fini(void);


static inline
void *
shmem_transport_mmap_ptr(const void *target, int pe, int noderank)
{
    char *remote_ptr;

    MMAP_GET_REMOTE_ACCESS(target, noderank, remote_ptr);
    return remote_ptr;
}


static inline
void
shmem_transport_mmap_put(void *target, const void *source, size_t len,
                          int pe, int noderank)
{
    char *remote_ptr;

    MMAP_GET_REMOTE_ACCESS(target, noderank, remote_ptr);
#ifdef ENABLE_ERROR_CHECKING
    if (NULL == remote_ptr) {
        RAISE_ERROR_MSG("target (0x%"PRIXPTR") outside of symmetric areas\n",
                        (uintptr_t) target);
    }
#endif

    memcpy(remote_ptr, source, len);
}


static inline
void
shmem_transport_mmap_get(void *target, const void *source, size_t len,
                          int pe, int noderank)
{
    char *remote_ptr;

    MMAP_GET_REMOTE_ACCESS(source, noderank, remote_ptr);
#ifdef ENABLE_ERROR_CHECKING
    if (NULL == remote_ptr) {
        RAISE_ERROR_MSG("target (0x%"PRIXPTR") outside of symmetric areas\n",
                        (uintptr_t) target);
    }
#endif

    memcpy(target, remote_ptr, len);
}

#endif
