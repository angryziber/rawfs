/**
 * RAW photo filesystem (based on fuse) that shows raw photos as their embedded jpeg thumbnails
 * Tested with Canon CR2 (NEF, PEF, etc need some fixes)
 * Written by Anton Keks
 * Licensed under GPLv3
 */

#define FUSE_USE_VERSION 26

#define _DEFAULT_SOURCE
#define TRUE 1
#define FALSE 0

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <dirent.h>
#include <sys/time.h>
#include <stdbool.h>
#include <signal.h>
#include <execinfo.h>

#include "raw.c"

FILE *flog = NULL;
char *photos_path = NULL;

int jpeg_size(const char *path) {
  struct img_data img;
  img.out_length = 0;
	int fd = open(path, O_RDONLY);
  if (fd != -1) {
    int res = parse_raw(fd, &img);
		close(fd);
		if (res < 0) return res;
  }
  return img.out_length;
}

int ends_with(const char *s, const char *ending) {
	size_t slen = strlen(s);
	size_t elen = strlen(ending);
	for (int i = 1; i <= elen; i++) {
		if (s[slen-i] != ending[elen-i])
			return FALSE;
	}
	return TRUE;
}

char *to_real_path(char *dest, const char *path) {
	if (path[0] == '/')
		sprintf(dest, "%s%s", photos_path, path);
	else
		sprintf(dest, "%s", path);

	if (ends_with(dest, ".CR2.jpg") || ends_with(dest, ".cr2.jpg"))
		dest[strlen(dest)-4] = 0;
	return dest;
}

bool is_supported_file(const char *path) {
	return ends_with(path, ".CR2") || ends_with(path, ".cr2");
}

static int rawfs_getattr(const char *path, struct stat *stbuf) {
	char new_path[PATH_MAX];
	path = to_real_path(new_path, path);

	int res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	if (S_ISREG(stbuf->st_mode) && is_supported_file(path))
		stbuf->st_size = jpeg_size(path);
	return 0;
}

static int rawfs_readlink(const char *path, char *buf, size_t size) {
	char new_path[PATH_MAX];
	path = to_real_path(new_path, path);

	int res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}

static int rawfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	char new_path[PATH_MAX];
	path = to_real_path(new_path, path);
	DIR *dp = opendir(path);
	if (dp == NULL)
		return -errno;

	struct stat st;
	memset(&st, 0, sizeof(st));

	struct dirent *de;
	while ((de = readdir(dp)) != NULL) {
		char *path = de->d_name;
	
		if (de->d_type != DT_DIR && is_supported_file(path))
			sprintf(path = (char*)&new_path, "%s.jpg", de->d_name);

		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
	
		if (filler(buf, path, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int rawfs_release(const char *path, struct fuse_file_info *fi) {
  struct img_data *img = (struct img_data*)(intptr_t)fi->fh;

	if (flog) {
    fprintf(flog, "rawfs_release %s\n", path);
    fflush(flog);
	}

  if (img->fd) close(img->fd);
  if (img->out) free(img->out);
  free(img);
	return 0;
}

static int rawfs_open(const char *path, struct fuse_file_info *fi) {
	char new_path[PATH_MAX];
	path = to_real_path(new_path, path);

	if (flog) {
    fprintf(flog, "rawfs_open %s\n", path);
    fflush(flog);
	}

	int fd = open(path, fi->flags);
	if (fd == -1) return -errno;

	struct img_data *img = malloc(sizeof *img);
	fi->fh = (uintptr_t) img;

  if (is_supported_file(path)) {
    int res = prepare_jpeg(fd, img);
    if (res < 0 && res != -1) {
      rawfs_release(path, fi);
      return res;
    }
  }
  else {
    img->fd = fd;
    img->out = 0;
    img->out_length = 0;
  }

	return 0;
}

static bool is_passthrough(struct img_data* img) {
  return img->out_length == 0;
}

static int rawfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	struct img_data *img = (struct img_data*)(intptr_t)fi->fh;
	if (flog) {
    fprintf(flog, "rawfs_read %zu %zu %u\n", size, offset, img->out_length);
    fflush(flog);
	}
	
	if (is_passthrough(img)) {
		return pread(img->fd, buf, size, offset);
	}
	else {
		if (offset > img->out_length) return 0;
		if (offset + size > img->out_length) size = img->out_length - offset;
		memcpy(buf, img->out + offset, size);
		return size;
	}
}

static int rawfs_unlink(const char* path) {
  char real_path[PATH_MAX];
  path = to_real_path(real_path, path);
  return unlink(path) == 0 ? 0 : -errno;
}

static int rawfs_rmdir(const char* path) {
  char real_path[PATH_MAX];
  path = to_real_path(real_path, path);
  return rmdir(path) == 0 ? 0 : -errno;
}

static int rawfs_mkdir(const char* path, mode_t mode) {
  char real_path[PATH_MAX];
  path = to_real_path(real_path, path);
  return mkdir(path, mode) == 0 ? 0 : -errno;
}

static int rawfs_rename(const char* from, const char* to) {
  char real_from[PATH_MAX], real_to[PATH_MAX];
  from = to_real_path(real_from, from);
  to = to_real_path(real_to, to);
  return rename(from, to) == 0 ? 0 : -errno;
}

static int rawfs_symlink(const char *from, const char *to) {
  char real_from[PATH_MAX], real_to[PATH_MAX];
  from = to_real_path(real_from, from);
  to = to_real_path(real_to, to);
  return symlink(from, to) == 0 ? 0 : -errno;
}

static struct fuse_operations rawfs_oper = {
	.getattr  = rawfs_getattr,
	.readlink = rawfs_readlink,
	.readdir  = rawfs_readdir,
	.open     = rawfs_open,
	.release  = rawfs_release,
	.read     = rawfs_read,
	.unlink   = rawfs_unlink,
	.rmdir    = rawfs_rmdir,
	.mkdir    = rawfs_mkdir,
	.rename	  = rawfs_rename,
	.symlink  = rawfs_symlink,
	.flag_nullpath_ok = 1
};

void crash_handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main(int argc, char *argv[]) {
  signal(SIGSEGV, crash_handler);

  if (argc < 3)
    fprintf(stderr, "usage: %s original_dir mountpoint [options]\n", argv[0]);

  if (argc == 2 && argv[1][0] == '-')
    return fuse_main(argc, argv, &rawfs_oper, NULL);

  if (argc < 3)
    return 1;

//    flog = fopen("rawfs.log", "wt");

  photos_path = realpath(argv[1], NULL);
  if (!photos_path) {
    fprintf(stderr, "rawfs: cannot read %s\n", argv[1]);
    return 2;
  }

  umask(0);
  return fuse_main(argc-1, argv+1, &rawfs_oper, NULL);
}
