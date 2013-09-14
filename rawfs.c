/**
 * RAW photo filesystem (based on fuse) that shows raw photos as their embedded jpeg thumbnails
 * Tested with Canon CR2 (NEF, PEF, etc testing needed)
 * Written by Anton Keks
 * Licensed under GPLv3
 */

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
#include <dirent.h>
#include <sys/time.h>

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

char *to_real_path(char *dest, const char *path)  {
	sprintf(dest, "%s%s", photos_path, path);
	if (ends_with(dest, ".jpg"))
		dest[strlen(dest)-4] = 0;
	return dest;
}

static int rawfs_getattr(const char *path, struct stat *stbuf) {
	char new_path[PATH_MAX];
	path = to_real_path(new_path, path);

	int res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

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

	struct dirent *de;
	while ((de = readdir(dp)) != NULL) {
		if (de->d_type != DT_DIR && !(ends_with((char*)&de->d_name, ".CR2") || ends_with((char*)&de->d_name, ".cr2")))
			continue;
	
		sprintf((char*)&new_path, "%s.jpg", de->d_name);
		if (filler(buf, de->d_type != DT_DIR ? new_path : de->d_name, NULL, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int rawfs_release(const char *path, struct fuse_file_info *fi) {
    struct img_data *img = (struct img_data*)(intptr_t)fi->fh;
    free(img->out);
    free(img);
	return 0;
}

static int rawfs_open(const char *path, struct fuse_file_info *fi) {
	char new_path[PATH_MAX];
	path = to_real_path(new_path, path);

	int fd = open(path, fi->flags);
	if (fd == -1)
		return -errno;
		
	struct img_data *img = malloc(sizeof *img);
	int res = prepare_jpeg(fd, img);
	fi->fh = (uintptr_t)img;
	
	if (flog) {
	    fprintf(flog, "rawfs_open %s\n", path);
	    fflush(flog);
	}

	close(fd);
	if (res < 0) {
	    rawfs_release(path, fi);
	    return res;	    
	}
	return 0;
}

static int rawfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	if (flog) {
	    fprintf(flog, "rawfs_read %zu %zu %zu\n", fi->fh, size, offset);
	    fflush(flog);
	}

    struct img_data *img = (struct img_data*)(intptr_t)fi->fh;
    memcpy(buf, img->out + offset, size);
	return size;
}

static struct fuse_operations rawfs_oper = {
	.getattr	= rawfs_getattr,
	.readlink	= rawfs_readlink,
	.readdir	= rawfs_readdir,
	.open		= rawfs_open,
	.release    = rawfs_release,
	.read		= rawfs_read
};

int main(int argc, char *argv[]) {
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

