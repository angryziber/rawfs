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
};

void find_thumb(int fd, struct img_data *img) {
    memset(img, 0, sizeof *img);
    
	lseek(fd, 16, SEEK_SET);
	short ifd_size = 0;
	read(fd, &ifd_size, 2);
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

	img->out_length = 12 + img->thumb_length-2 + img->ifd2_offset;
		
	char *out = malloc(img->out_length);
	memcpy(out, "\xff\xd8\xff\xe1  Exif\0\0", 12);   // jpeg/exif header
	*(short*)&out[4] = htons(2 + img->ifd2_offset);  // exif data size
	
	lseek(fd, 0, SEEK_SET);
	read(fd, out+12, img->ifd2_offset); // tiff header + exif data + etc
	*(int*)&out[12 + 16 + 2 + ifd_size * sizeof *tags] = 0;
		
	lseek(fd, img->thumb_offset+2, SEEK_SET);
	read(fd, out+12 + img->ifd2_offset, img->thumb_length-2);
	
	write_file("thumb_exif.jpg", out, img->out_length);
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

