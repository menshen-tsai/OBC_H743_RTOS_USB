#ifndef LITTLEFS_QSPI_H
#define LITTLEFS_QSPI_H

#include "lfs.h"

// Expose the config so application can mount
extern struct lfs_config qspi_lfs_cfg;

int qspi_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);

int qspi_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int qspi_erase(const struct lfs_config *c, lfs_block_t block);
int qspi_sync(const struct lfs_config *c);

#endif
