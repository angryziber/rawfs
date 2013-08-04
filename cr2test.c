#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned short ushort;
typedef unsigned int uint;

struct tiff_tag {
  ushort id, type;
  int count;
  union { char c[4]; short s[2]; int i; } val;
};

void find_thumb(FILE *f, int *thumb_offset, int *thumb_length) {
	fseek(f, 16, SEEK_SET);
	short ifd_size = 0;
	fread(&ifd_size, 1, 2, f);
    struct tiff_tag tag;
	for (int i = 0; i < ifd_size; i++) {
	    fread(&tag, 1, sizeof tag, f);
	    if (tag.id == 0x111) *thumb_offset = tag.val.i;
	    else if (tag.id == 0x117) *thumb_length = tag.val.i;
	}
}

int main(int argc, char* argv[]) {
	FILE* f = fopen(argv[1], "rb");
    if (f) {
		int thumb_offset = 0;
		int thumb_length = 0;
		find_thumb(f, &thumb_offset, &thumb_length);
		
		printf("thumb_offset 0x%x, thumb_length %d\n", thumb_offset, thumb_length);
		
		fseek(f, thumb_offset, SEEK_SET);
		char *thumb = malloc(thumb_length);
		fread(thumb, 1, thumb_length, f);
		
		char *filename = "thumb.jpg";
		FILE* ofp = fopen(filename, "wb");
		fwrite(thumb, 1, thumb_length, ofp);
		fclose(ofp);
		printf("Written %s\n", filename);
    }
    return 0;
}

