/* 
 * Common raw parsing code
 * Written by Anton Keks
 * CR2 spec:  http://lclevy.free.fr/cr2/
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
  struct tiff_head header;
  short ifd_size;
  off_t thumb_offset;
  size_t thumb_length;
  off_t ifd2_offset;
  size_t out_length;
  char *out;
};

int parse_raw(int fd, struct img_data *img) {
    memset(img, 0, sizeof *img);
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

	for (int i = 0; i < img->ifd_size; i++) {
	    struct tiff_tag *tag = &tags[i];
	    if (tag->id == 0x111) img->thumb_offset = tag->val.i;
	    else if (tag->id == 0x117) img->thumb_length = tag->val.i;
	}

	res = read(fd, &img->ifd2_offset, 4);
  	if (res == -1) return -errno;

	img->out_length = EXIF_HEADER_LENGTH + img->thumb_length-2 + img->ifd2_offset;
	return 0;
}

int prepare_jpeg(int fd, struct img_data *img) {
    size_t res = 0;
    res = parse_raw(fd, img);
   	if (res < 0) return res;

	char *outp = img->out = malloc(img->out_length);
	
	memcpy(outp, EXIF_HEADER, EXIF_HEADER_LENGTH);
	*(short*)&outp[4] = htons(8 + img->ifd2_offset);  // exif data size in the APP1 marker
	outp += EXIF_HEADER_LENGTH;

	res = pread(fd, outp, img->ifd2_offset, 0); // tiff header + exif data + etc
	if (res == -1) return -errno;

	int ifd_length = sizeof img->ifd_size + img->ifd_size * sizeof(struct tiff_tag);
	*(int*)&outp[img->header.length + ifd_length] = 0;
	outp += img->ifd2_offset;
		
	res = pread(fd, outp, img->thumb_length-2, img->thumb_offset+2); // skip 2 bytes - jpeg marker that is already included in EXIF_HEADER
	if (res == -1) return -errno;
	return 0;
}

void write_file(const char* path, const char* data, int len) {
    FILE* ofp = fopen(path, "wb");
    fwrite(data, 1, len, ofp);
    fclose(ofp);
    printf("Written %s\n", path);
}

