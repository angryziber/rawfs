/*
 * Command-line utility to extract jpeg thumbnail from raw photo
 * Minimalistic version, much smaller than similar code in dcraw.c
 * Written by Anton Keks
 */

#include "raw.c"

void write_file(const char* path, const char* data, int len) {
	FILE* ofp = fopen(path, "wb");
	fwrite(data, 1, len, ofp);
	fclose(ofp);
	printf("Written %s\n", path);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s photo\n", argv[0]);
        return 1;
    }

   	struct img_data img;
	int fd = open(argv[1], O_RDONLY);
    if (fd != -1) {
        prepare_jpeg(fd, &img);        
       	write_file("thumb_exif.jpg", img.out, img.out_length);
    	free(img.out);
		close(fd);
		printf("thumb_offset 0x%x, thumb_length %d", img.thumb_offset, img.thumb_length);
    }
    return 0;
}
