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
#include <fcntl.h>
#include <arpa/inet.h>

#define EXIF_HEADER "\xff\xd8\xff\xe1  Exif\0\0"
#define EXIF_HEADER_LENGTH 12
#define CR2_HEADER_LENGTH 16

typedef unsigned short ushort;
typedef unsigned int uint;

struct tiff_tag {
  ushort id, type;
  int count;
  union { char c[4]; short s[2]; int i; } val;
};

struct img_data {
  short ifd_size;
  int thumb_offset;
  int thumb_length;
  int ifd2_offset;
  int out_length;
  char *out;
};

int parse_raw(int fd, struct img_data *img) {
    memset(img, 0, sizeof *img);
    int res = 0;
    
	lseek(fd, CR2_HEADER_LENGTH, SEEK_SET);
	res = read(fd, &img->ifd_size, sizeof img->ifd_size);
	if (res < 0) return res;
	
    struct tiff_tag tags[img->ifd_size];
    res = read(fd, tags, img->ifd_size * sizeof *tags);
  	if (res < 0) return res;

	for (int i = 0; i < img->ifd_size; i++) {
	    struct tiff_tag *tag = &tags[i];
	    if (tag->id == 0x111) img->thumb_offset = tag->val.i;
	    else if (tag->id == 0x117) img->thumb_length = tag->val.i;
	}

	res = read(fd, &img->ifd2_offset, 4);
	img->out_length = EXIF_HEADER_LENGTH + img->thumb_length-2 + img->ifd2_offset;
	return res;
}

int prepare_jpeg(int fd, struct img_data *img) {
    int res = 0;
    res = parse_raw(fd, img);
   	if (res < 0) return res;

	char *outp = img->out = malloc(img->out_length);
	
	memcpy(outp, EXIF_HEADER, EXIF_HEADER_LENGTH);
	*(short*)&outp[4] = htons(8 + img->ifd2_offset);  // exif data size in the APP1 marker
	outp += EXIF_HEADER_LENGTH;

	res = pread(fd, outp, img->ifd2_offset, 0); // tiff header + exif data + etc
	if (res < 0) return res;

	int ifd_length = sizeof img->ifd_size + img->ifd_size * sizeof(struct tiff_tag);
	*(int*)&outp[CR2_HEADER_LENGTH + ifd_length] = 0;
	outp += img->ifd2_offset;
		
	return pread(fd, outp, img->thumb_length-2, img->thumb_offset+2); // skip 2 bytes - jpeg marker that is already included in EXIF_HEADER
}

