#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

// http://lclevy.free.fr/cr2/
// http://www.media.mit.edu/pia/Research/deepview/exif.html


typedef unsigned short ushort;
typedef unsigned int uint;

void write_file(const char* path, const char* data, int len) {
	FILE* ofp = fopen(path, "wb");
	fwrite(data, 1, len, ofp);
	fclose(ofp);
	printf("Written %s\n", path);
}

struct tiff_tag {
  ushort id, type;
  int count;
  union { char c[4]; short s[2]; int i; } val;
};

struct img_data {
  int thumb_offset;
  int thumb_length;
  int exif_offset;
  int ifd2_offset;
  int out_length;
  char *out;
};

#define EXIF_HEADER "\xff\xd8\xff\xe1  Exif\0\0"
#define EXIF_HEADER_LENGTH 12
#define CR2_HEADER_LENGTH 16

void find_thumb(int fd, struct img_data *img) {
    memset(img, 0, sizeof *img);
    
	lseek(fd, CR2_HEADER_LENGTH, SEEK_SET);
	short ifd_size = 0;
	read(fd, &ifd_size, sizeof ifd_size);
    struct tiff_tag tags[ifd_size];
    read(fd, tags, ifd_size * sizeof *tags);
	for (int i = 0; i < ifd_size; i++) {
	    struct tiff_tag *tag = &tags[i];
	    if (tag->id == 0x111) img->thumb_offset = tag->val.i;
	    else if (tag->id == 0x117) img->thumb_length = tag->val.i;
	    else if (tag->id == 0x8769) img->exif_offset = tag->val.i;
	}
	
	read(fd, &img->ifd2_offset, 4);
	printf("ifd2_offset %d\n", img->ifd2_offset);

	img->out_length = EXIF_HEADER_LENGTH + img->thumb_length-2 + img->ifd2_offset;		
	char *outp = img->out = malloc(img->out_length);
	
	memcpy(outp, EXIF_HEADER, EXIF_HEADER_LENGTH);
	*(short*)&outp[4] = htons(8 + img->ifd2_offset);  // exif data size in the APP1 marker
	outp += EXIF_HEADER_LENGTH;

	lseek(fd, 0, SEEK_SET);
	read(fd, outp, img->ifd2_offset); // tiff header + exif data + etc
	*(int*)&outp[CR2_HEADER_LENGTH + sizeof ifd_size + ifd_size * sizeof *tags] = 0;
	outp += img->ifd2_offset;
		
	lseek(fd, img->thumb_offset+2, SEEK_SET); // skip 2 bytes - jpeg marker that is already included in EXIF_HEADER
	read(fd, outp, img->thumb_length-2);
	
	write_file("thumb_exif.jpg", img->out, img->out_length);
	free(img->out);
}

int main(int argc, char* argv[]) {
   	struct img_data img;
	int fd = open(argv[1], O_RDONLY);
    if (fd != -1) {
    	find_thumb(fd, &img);
		printf("thumb_offset 0x%x, thumb_length %d, exif_offset %d\n", img.thumb_offset, img.thumb_length, img.exif_offset);
		
		lseek(fd, img.thumb_offset, SEEK_SET);
		char *thumb = malloc(img.thumb_length);
		read(fd, thumb, img.thumb_length);
		
		write_file("thumb.jpg", thumb, img.thumb_length);
		close(fd);
    }
    return 0;
}

