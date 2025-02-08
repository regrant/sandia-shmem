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

#include "config.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SHMEM_INTERNAL_INCLUDE
#include "shmem.h"
#include "shmem_internal.h"
#include "shmem_comm.h"
#include "runtime.h"
#include "transport_mmap.h"

#define MPIDI_OFI_SHMGR_NAME_MAXLEN (128)
#define MPIDI_OFI_SHMGR_NAME_PREFIX "/sos_shm_mmap_area"


static void shm_create_key(char *key, size_t max_size, unsigned pe, size_t num) {
    snprintf(key, max_size, "%s-%u-%zu", MPIDI_OFI_SHMGR_NAME_PREFIX, pe, num);
}


static void *shm_create_region(char* base, const char *key, int shm_size) {
  if (shm_size == 0) return NULL;

  shm_unlink(key);
  int fd = shm_open(key, O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (fd == -1) {
      fprintf(stderr, "mmap_init error shm_open with errno(%s)\n", strerror(errno));
      exit(0);
  }

  if (ftruncate(fd, shm_size) == -1) {
      fprintf(stderr, "mmap_init error ftruncate\n");
      exit(0);
  }

  void *shm_base_addr = mmap(base, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
  if (MAP_FAILED == shm_base_addr) {
      fprintf(stderr, "mmap_init error mmap %s size %d\n", key, shm_size);
      exit(0);
  }

  return shm_base_addr;
}


static void *shm_create_region_data_seg(char* base, const char *key, int shm_size) {
    if (shm_size == 0) return NULL;

    shm_unlink(key);
    int fd = shm_open(key, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        fprintf(stderr, "mmap_init data_seg error shm_open with errno(%s)\n", strerror(errno));
        exit(1);
    }

    /* Write all current contents of the data segment to the file */
    FILE *fp = fdopen(fd, "wb");
    size_t ret = fwrite(base, shm_size, 1, fp);

    if (ret == 0) {
        fprintf(stderr, "mmap_init error fwrite\n");
        exit(1);
    }

    if (ftruncate(fd, shm_size) == -1) {
        fprintf(stderr, "mmap_init error ftruncate with errno(%s)\n", strerror(errno));
        exit(1);
    }

    void *shm_base_addr = mmap(base, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
    if (MAP_FAILED == shm_base_addr) {
        fprintf(stderr, "mmap_init error mmap %s size %d\n", key, shm_size);
        exit(1);
    }

    fclose(fp);

    return shm_base_addr;
}


static void *shm_attach_region(char* base, const char *key, int shm_size) {
  if (shm_size == 0) return NULL;

  int fd = shm_open(key, O_RDWR, 0);
  if (fd == -1) {
      fprintf(stderr, "mmap_init error shm_open\n");
      exit(0);
  }
  void *shm_base_addr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (MAP_FAILED == shm_base_addr) {
      fprintf(stderr, "mmap_init error mmap %s  size %d\n", key, shm_size);
      exit(0);
  }
  return shm_base_addr;
}


struct share_info_t {
    size_t data_len;
    size_t data_off;
    size_t heap_len;
    size_t heap_off;
};

struct shmem_transport_mmap_peer_info_t *shmem_transport_mmap_peers = NULL;
static struct share_info_t my_info;

#define FIND_BASE(ptr, page_size) ((char*) (((uintptr_t) ptr / page_size) * page_size))
#define FIND_LEN(ptr, len, page_size) ((((char*) ptr - FIND_BASE(ptr, page_size) + len - 1) / \
                                        page_size + 1) * page_size)

int
shmem_transport_mmap_init(void)
{
    long page_size = sysconf(_SC_PAGESIZE);
    char *base;
    size_t len;
    int ret;
    char key_prefix[MPIDI_OFI_SHMGR_NAME_MAXLEN-10];
    char key[MPIDI_OFI_SHMGR_NAME_MAXLEN];

    /* setup data region */
    base = FIND_BASE(shmem_internal_data_base, page_size);
    len = FIND_LEN(shmem_internal_data_base, shmem_internal_data_length, page_size);
    shm_create_key(key_prefix, MPIDI_OFI_SHMGR_NAME_MAXLEN-10, shmem_internal_my_pe, 1);
    snprintf(key, MPIDI_OFI_SHMGR_NAME_MAXLEN, "%s-data", key_prefix);
    void* myaddr_data = shm_create_region_data_seg(base, key, len);
    if (myaddr_data == NULL) return 1;

    my_info.data_off = (char*) shmem_internal_data_base - (char*) base;
    my_info.data_len = len;

    /* setup heap region */
    base = FIND_BASE(shmem_internal_heap_base, page_size);
    len  = FIND_LEN(shmem_internal_heap_base, shmem_internal_heap_length, page_size);
    shm_create_key(key_prefix, MPIDI_OFI_SHMGR_NAME_MAXLEN-10, shmem_internal_my_pe, 2);
    snprintf(key, MPIDI_OFI_SHMGR_NAME_MAXLEN, "%s-heap", key_prefix);
    void* myaddr_heap = shm_create_region(base, key, len);
    if (myaddr_heap == NULL) return 1;

    my_info.heap_off = (char*) shmem_internal_heap_base - (char*) base;
    my_info.heap_len = len;

    ret = shmem_runtime_put("mmap-segids", &my_info, sizeof(struct share_info_t));
    if (0 != ret) {
        RETURN_ERROR_MSG("runtime_put failed: %d\n", ret);
        return 1;
    }

    return 0;
}


int
shmem_transport_mmap_startup(void)
{
    int ret, peer_num, num_on_node;
    char errmsg[256];
    struct share_info_t info;
    //struct mmap_addr addr;
    long page_size = sysconf(_SC_PAGESIZE);

    num_on_node = shmem_runtime_get_node_size();

    /* allocate space for local peers */
    shmem_transport_mmap_peers = calloc(num_on_node,
                                         sizeof(struct shmem_transport_mmap_peer_info_t));
    if (NULL == shmem_transport_mmap_peers) return 1;

    /* get local peer info and map into our address space ... */
    for (int i = 0 ; i < shmem_internal_num_pes; ++i) {
        peer_num = shmem_runtime_get_node_rank(i);
        if (-1 == peer_num) continue;

        if (shmem_internal_my_pe == i) {
            shmem_transport_mmap_peers[peer_num].data_ptr =
                shmem_internal_data_base;
            shmem_transport_mmap_peers[peer_num].heap_ptr =
                shmem_internal_heap_base;
        } else {
            ret = shmem_runtime_get(i, "mmap-segids", &info, sizeof(struct share_info_t));
            if (0 != ret) {
                RETURN_ERROR_MSG("runtime_get failed: %d\n", ret);
                return 1;
            }

            char key_prefix[MPIDI_OFI_SHMGR_NAME_MAXLEN-10];
            char key[MPIDI_OFI_SHMGR_NAME_MAXLEN];
            int len = 0;

	    /* Attach data segment to neighbors: */
            len  = FIND_LEN(shmem_internal_data_base, shmem_internal_data_length, page_size);
            shm_create_key(key_prefix, MPIDI_OFI_SHMGR_NAME_MAXLEN-10, i, 1);
            snprintf(key, MPIDI_OFI_SHMGR_NAME_MAXLEN, "%s-data", key_prefix);
            shmem_transport_mmap_peers[peer_num].data_attach_ptr = shm_attach_region(NULL, key, len);

            if (shmem_transport_mmap_peers[peer_num].data_attach_ptr == NULL) {
                RETURN_ERROR_MSG("could not get data segment: %s\n",
                                 shmem_util_strerror(errno, errmsg, 256));
                return 1;
            }
            shmem_transport_mmap_peers[peer_num].data_ptr =
                (char*) shmem_transport_mmap_peers[peer_num].data_attach_ptr + info.data_off;

            /* Attach heap segment to neighbors: */
            len  = FIND_LEN(shmem_internal_heap_base, shmem_internal_heap_length, page_size);
            shm_create_key(key_prefix, MPIDI_OFI_SHMGR_NAME_MAXLEN-10, i, 2);
            snprintf(key, MPIDI_OFI_SHMGR_NAME_MAXLEN, "%s-heap", key_prefix);
            shmem_transport_mmap_peers[peer_num].heap_attach_ptr = shm_attach_region(NULL, key, len);

            if (shmem_transport_mmap_peers[peer_num].heap_attach_ptr == NULL) {
                RETURN_ERROR_MSG("could not get heap segment: %s\n",
                                 shmem_util_strerror(errno, errmsg, 256));
                return 1;
            }
            shmem_transport_mmap_peers[peer_num].heap_ptr =
                (char*) shmem_transport_mmap_peers[peer_num].heap_attach_ptr + info.heap_off;
        }
    }

    return 0;
}


int
shmem_transport_mmap_fini(void)
{
    int i, peer_num, ret;
    char errmsg[256];
    size_t data_len, heap_len;
    char key_prefix[MPIDI_OFI_SHMGR_NAME_MAXLEN-10];
    char key[MPIDI_OFI_SHMGR_NAME_MAXLEN];
    long page_size = sysconf(_SC_PAGESIZE);

    data_len = FIND_LEN(shmem_internal_data_base, shmem_internal_data_length, page_size);
    heap_len = FIND_LEN(shmem_internal_heap_base, shmem_internal_heap_length, page_size);

    shm_create_key(key_prefix, MPIDI_OFI_SHMGR_NAME_MAXLEN-10, shmem_internal_my_pe, 1);
    snprintf(key, MPIDI_OFI_SHMGR_NAME_MAXLEN, "%s-data", key_prefix);

    ret = shm_unlink(key);
    if (ret != 0) {
        RETURN_ERROR_MSG("could not get data segment: %s\n", \
                         shmem_util_strerror(errno, errmsg, 256));
    }

    shm_create_key(key_prefix, MPIDI_OFI_SHMGR_NAME_MAXLEN-10, shmem_internal_my_pe, 2);
    snprintf(key, MPIDI_OFI_SHMGR_NAME_MAXLEN, "%s-heap", key_prefix);

    ret = shm_unlink(key);
    if (ret != 0) {
        RETURN_ERROR_MSG("could not get heap segment: %s\n", \
                         shmem_util_strerror(errno, errmsg, 256));
    }

    if (NULL != shmem_transport_mmap_peers) {
        for (i = 0 ; i < shmem_internal_num_pes; ++i) {
            peer_num = shmem_runtime_get_node_rank(i);
            if (-1 == peer_num) continue;
            if (shmem_internal_my_pe == i) continue;

            if (NULL != shmem_transport_mmap_peers[peer_num].data_attach_ptr) {
                munmap(shmem_transport_mmap_peers[peer_num].data_attach_ptr, data_len);
            }

            if (NULL != shmem_transport_mmap_peers[peer_num].heap_attach_ptr) {
                munmap(shmem_transport_mmap_peers[peer_num].heap_attach_ptr, heap_len);
            }
        }
        free(shmem_transport_mmap_peers);
    }

    return 0;
}
