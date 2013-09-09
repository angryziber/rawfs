#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

typedef unsigned short ushort;
typedef unsigned int uint;

struct tiff_tag {
  ushort id, type;
  int count;
  union { char c[4]; short s[2]; int i; } val;
};

void find_thumb(int fd, int *thumb_offset, int *thumb_length) {
	lseek(fd, 16, SEEK_SET);
	short ifd_size = 0;
	read(fd, &ifd_size, 2);
    struct tiff_tag tags[ifd_size];
    read(fd, tags, ifd_size * sizeof *tags);
	for (int i = 0; i < ifd_size; i++) {
	    struct tiff_tag *tag = &tags[i];
	    if (tag->id == 0x111) *thumb_offset = tag->val.i;
	    else if (tag->id == 0x117) *thumb_length = tag->val.i;
	}
}

int main(int argc, char* argv[]) {
   	int thumb_offset = 0;
    int thumb_length = 0;
	int fd = open(argv[1], O_RDONLY);
    if (fd != -1) {
    	find_thumb(fd, &thumb_offset, &thumb_length);
		printf("thumb_offset 0x%x, thumb_length %d\n", thumb_offset, thumb_length);
		
		lseek(fd, thumb_offset, SEEK_SET);
		char *thumb = malloc(thumb_length);
		read(fd, thumb, thumb_length);
		
		char *filename = "thumb.jpg";
		FILE* ofp = fopen(filename, "wb");
		fwrite(thumb, 1, thumb_length, ofp);
		fclose(ofp);
		printf("Written %s\n", filename);

		close(fd);
    }
    return 0;
}

