/* 
 * Common raw parsing code
 * Written by Anton Keks
 * CR2 spec:  http://lclevy.free.fr/cr2/ (uses Intel byte order)
 * NEF spec:  http://lclevy.free.fr/nef/ (uses Motorola byte order)
 * Exif spec: http://www.media.mit.edu/pia/Research/deepview/exif.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define EXIF_HEADER "\xff\xd8\xff\xe1  Exif\0\0"
#define EXIF_HEADER_LENGTH 12

typedef unsigned short ushort;
typedef unsigned int uint;

struct tiff_head {
  short byte_order; // II or MM
  short magic; // always 0x002a
  short length;
};

struct tiff_tag {
  ushort id, type;
  int count;
  union { char c[4]; short s[2]; int i; } val;
};

struct img_data {
  int fd;
  struct tiff_head header;
  unsigned short ifd_size;
  unsigned int thumb_offset;
  unsigned int thumb_length;
  unsigned int exif_data_length;
  unsigned int out_length;
  char *out;
};

int parse_raw(int fd, struct img_data *img) {
    memset(img, 0, sizeof *img);
    img->fd = fd;
    size_t res = 0;
    
    read(fd, &img->header, sizeof img->header);
    if (img->header.byte_order != 0x4949 || img->header.magic != 0x002a)
        return -1;  // we support only Intel byte order
    
    lseek(fd, img->header.length, SEEK_SET);
    
	res = read(fd, &img->ifd_size, sizeof img->ifd_size);
	if (res == -1) return -errno;
	
    struct tiff_tag tags[img->ifd_size];
    res = read(fd, tags, img->ifd_size * sizeof *tags);
  	if (res == -1) return -errno;

    int exif_offset = 0x10e;
	for (int i = 0; i < img->ifd_size; i++) {
	    struct tiff_tag *tag = &tags[i];
	    if (tag->id == 0x111) img->thumb_offset = tag->val.i;
	    else if (tag->id == 0x117) img->thumb_length = tag->val.i;
	    else if (tag->id == 0x8769) exif_offset = tag->val.i;
	}
	
    short exif_num_entries = 0;
    lseek(fd, exif_offset, SEEK_SET);
	res = read(fd, &exif_num_entries, 2);
  	if (res == -1) return -errno;
  	
  	img->exif_data_length = exif_offset + exif_num_entries * sizeof *tags + 4;
	img->out_length = EXIF_HEADER_LENGTH + img->exif_data_length + img->thumb_length-2;
	return 0;
}

int copy_exif_header(char *outp, struct img_data *img) {
	memcpy(outp, EXIF_HEADER, EXIF_HEADER_LENGTH);
	*(short*)&outp[4] = htons(8 + img->exif_data_length);  // exif data size in the APP1 marker
	return EXIF_HEADER_LENGTH;
}

int copy_exif_data(char *outp, struct img_data *img) {
	int res = pread(img->fd, outp, img->exif_data_length, 0); // tiff header + exif data + etc
	if (res == -1) return -errno;

	int ifd_length = sizeof img->ifd_size + img->ifd_size * sizeof(struct tiff_tag);
	int next_ifd = img->header.length + ifd_length;
	*(int*)&outp[next_ifd] = 0;
	return res;
}

int copy_jpeg_data(char *outp, struct img_data *img) {
    // skip 2 bytes - jpeg marker that is already included in EXIF_HEADER
	int res = pread(img->fd, outp, img->thumb_length-2, img->thumb_offset+2);
	return res == -1 ? -errno : res;
}

int prepare_jpeg(int fd, struct img_data *img) {
    int res = parse_raw(fd, img);
   	if (res < 0) return res;

	char *outp = img->out = malloc(img->out_length);
	outp += copy_exif_header(outp, img);

    res = copy_exif_data(outp, img);
    if (res < 0) return res; else outp += res;
    
    res = copy_jpeg_data(outp, img);
    if (res < 0) return res; else outp += res;
		
	return 0;
}

void write_file(const char* path, const char* data, int len) {
    FILE* ofp = fopen(path, "wb");
    fwrite(data, 1, len, ofp);
    fclose(ofp);
    printf("Written %s\n", path);
}

