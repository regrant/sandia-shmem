/*                                                                                                                                                                                                                                           *  This test program is derived from an example program in the
 *  OpenSHMEM specification.
 */

#include <shmem.h>
#include <stdlib.h>
#include <stdio.h>

#define N_UPDATES (1lu << 18)
#define N_INDICES (1lu << 10)
#define N_VALUES  (1lu << 31)

int main(void) {

  shmem_init();

  uint64_t *table = shmem_calloc(N_INDICES, sizeof(uint64_t));

  int mype = shmem_my_pe();
  int npes = shmem_n_pes();
  srand(mype);

  shmem_ctx_t ctx;
  int ret = shmem_ctx_create(0, &ctx);
  if (ret != 0) {
      printf("%d: Error creating context (%d)\n", mype, ret);
      shmem_global_exit(1);
  }

  shmem_session_config_t config;
  long config_mask;
  config.total_ops = N_UPDATES;
  config_mask = SHMEM_SESSION_TOTAL_OPS;

  shmem_session_start(ctx, SHMEM_SESSION_SAME_AMO, &config, config_mask);

  for (size_t i = 0; i < N_UPDATES; i++) {
      int random_pe = rand() % npes;
      size_t random_idx = rand() % N_INDICES;
      uint64_t random_val = rand() % N_VALUES;
      shmem_ctx_uint64_atomic_xor(ctx, &table[random_idx], random_val, random_pe);
  }

  shmem_session_stop(ctx);
  shmem_ctx_quiet(ctx);    /* shmem_session_stop() does not quiet the context. */
  shmem_sync_all();        /* shmem_session_stop() does not synchronize.       */

  /* At this point, it is safe to check and/or validate the table result... */

  shmem_ctx_destroy(ctx);
  shmem_free(table);
  shmem_finalize();
  return 0;
}
