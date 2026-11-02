#ifndef DOOM_RAMFS_H
#define DOOM_RAMFS_H
#include <stddef.h>
#include <stdint.h>

// POSIX-style negative errno results. Bounded files, independent open offsets.
int ramfs_mount_wad(const char *name, const uint8_t *data, size_t size);
int ramfs_open(const char *name, int flags);
int ramfs_close(int fd);
int ramfs_read(int fd, void *data, size_t size);
int ramfs_write(int fd, const void *data, size_t size);
int ramfs_seek(int fd, int offset, int whence);
int ramfs_size(int fd);
int ramfs_unlink(const char *name);
int ramfs_rename(const char *oldname, const char *newname);

#endif
