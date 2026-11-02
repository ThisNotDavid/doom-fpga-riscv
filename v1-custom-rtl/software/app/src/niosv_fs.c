#include "ramfs.h"
#include "sys/alt_dev.h"
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

static int fs_open(alt_fd *fd, const char *name, int flags, int mode) {
	int h = ramfs_open(name, flags);
	(void) mode;
	if (h < 0)
		return h;
	fd->priv = (void*) (intptr_t)(h + 1);
	return 0;
}

static int hid(alt_fd *fd) {
	return (int) (intptr_t) fd->priv - 1;
}

static int fs_close(alt_fd *fd) {
	return ramfs_close(hid(fd));
}

static int fs_read(alt_fd *fd, char *p, int n) {
	return ramfs_read(hid(fd), p, n);
}

static int fs_write(alt_fd *fd, const char *p, int n) {
	return ramfs_write(hid(fd), p, n);
}

static int fs_seek(alt_fd *fd, int off, int whence) {
	return ramfs_seek(hid(fd), off, whence);
}

static int fs_stat(alt_fd *fd, struct stat *st) {
	memset(st, 0, sizeof(*st));
	st->st_mode = S_IFREG | 0600;
	st->st_size = ramfs_size(hid(fd));
	return 0;
}

static alt_dev device = { ALT_LLIST_ENTRY, "/ram", fs_open, fs_close, fs_read,
		fs_write, fs_seek, fs_stat, NULL };
		
void niosv_fs_init(void) {
	alt_fs_reg(&device);
}

static int result(int n) {
	if (n < 0) {
		errno = -n;
		return -1;
	}
	return n;
}

int _unlink(const char *name) {
	return result(ramfs_unlink(name));
}

int _rename(const char *oldname, const char *newname) {
	return result(ramfs_rename(oldname, newname));
}

int mkdir(const char *name, mode_t mode) {
	(void) mode;
	if (!strncmp(name, "/ram", 4))
		return 0;
	errno = ENOENT;
	return -1;
}
