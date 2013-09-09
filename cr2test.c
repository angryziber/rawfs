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

void find_thumb(int fd, int *thumb_offset, int *thumb_length, int *exif_offset) {
	lseek(fd, 16, SEEK_SET);
	short ifd_size = 0;
	read(fd, &ifd_size, 2);
    struct tiff_tag tags[ifd_size];
    read(fd, tags, ifd_size * sizeof *tags);
	for (int i = 0; i < ifd_size; i++) {
	    struct tiff_tag *tag = &tags[i];
	    if (tag->id == 0x111) *thumb_offset = tag->val.i;
	    else if (tag->id == 0x117) *thumb_length = tag->val.i;
	    else if (tag->id == 0x8769) *exif_offset = tag->val.i;
	}
	
	int next_ifd = 0;
	read(fd, &next_ifd, 4);
	printf("next_ifd %d\n", next_ifd);
		
	int exif_length = next_ifd - *exif_offset;
	
	lseek(fd, *exif_offset, SEEK_SET);
	read(fd, &ifd_size, 2);
	printf("ifd_size %d\n", ifd_size);

    struct tiff_tag etags[ifd_size];
	read(fd, etags, ifd_size * sizeof *etags);

	for (int i = 0; i < ifd_size; i++) {
	    struct tiff_tag *tag = &etags[i];
	    printf("exif 0x%x %d, %d\n", tag->id, tag->count, tag->val.i);
	}
	
	char *data = malloc(10 + *thumb_length + exif_length);
	memcpy(data, "\xff\xd8\xff\xe1  Exif\0\0", 12);   // jpeg/exif starter
	*(short*)&data[4] = htons(8 + exif_length);
	memcpy(data+12, "II\x2a\x00\x08\x00\x00\x00", 8); // tiff header
	
	lseek(fd, *exif_offset, SEEK_SET);
	read(fd, data+20, exif_length);
	
	lseek(fd, *thumb_offset+2, SEEK_SET);
	read(fd, data+20 + exif_length, *thumb_length-2);
	
	write_file("thumb_exif.jpg", data, 10 + *thumb_length + exif_length);
}

int main(int argc, char* argv[]) {
   	int thumb_offset = 0;
    int thumb_length = 0;
    int exif_offset = 0;
	int fd = open(argv[1], O_RDONLY);
    if (fd != -1) {
    	find_thumb(fd, &thumb_offset, &thumb_length, &exif_offset);
		printf("thumb_offset 0x%x, thumb_length %d, exif_offset %d\n", thumb_offset, thumb_length, exif_offset);
		
		lseek(fd, thumb_offset, SEEK_SET);
		char *thumb = malloc(thumb_length);
		read(fd, thumb, thumb_length);
		
		write_file("thumb.jpg", thumb, thumb_length);
		close(fd);
    }
    return 0;
}

