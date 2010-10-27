#define FUSE_USE_VERSION 26

#define _BSD_SOURCE
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>

const char *photos_path = "/home/anton/Photos";

char *to_real_path(char *dest, const char *path) 
{
	sprintf(dest, "%s%s", photos_path, path);
	return dest;
}

int ends_with(const char *s, const char *ending)
{
	size_t slen = strlen(s);
	size_t elen = strlen(ending);
	for (int i = 1; i <= elen; i++) {
		if (s[slen-i] != ending[elen-i])
			return FALSE;
	}
	return TRUE;
}

static int rawfs_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char new_path[2048];
	path = to_real_path(new_path, path);

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int rawfs_readlink(const char *path, char *buf, size_t size)
{
	int res;
	char new_path[2048];
	path = to_real_path(new_path, path);

	res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int rawfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;
	char new_path[2048];

	(void) offset;
	(void) fi;

	path = to_real_path(new_path, path);
	dp = opendir(path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		
		if (de->d_type != DT_DIR && !(ends_with((char*)&de->d_name, ".CR2") || ends_with((char*)&de->d_name, ".cr2")))
			continue;
		
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		sprintf((char*)&new_path, "%s.jpg", de->d_name);
		if (filler(buf, de->d_type != DT_DIR ? new_path : de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int rawfs_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	char new_path[2048];

	path = to_real_path(new_path, path);
	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int rawfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	char new_path[2048];

	(void) fi;
	path = to_real_path((char*)&new_path, path);
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static struct fuse_operations rawfs_oper = {
	.getattr	= rawfs_getattr,
	.readlink	= rawfs_readlink,
	.readdir	= rawfs_readdir,
	.open		= rawfs_open,
	.read		= rawfs_read
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &rawfs_oper, NULL);
}

