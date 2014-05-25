/*
 * Command-line utility to extract jpeg thumbnail from raw photo
 * Minimalistic version, much smaller than similar code in dcraw.c
 * Written by Anton Keks
 */

#include "raw.c"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s photo\n", argv[0]);
        return 1;
    }

   	struct img_data img;
	int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 1;
    }
    
    int res = prepare_jpeg(fd, &img);
    if (res < 0) {
        fprintf(stderr, "cannot parse %s\n", argv[1]);
        return 2;
    }
	close(fd);

   	write_file("thumb_exif.jpg", img.out, img.out_length);
	free(img.out);
	printf("thumb_offset 0x%x, thumb_length %u, ifd_size %u, exif_data_length %u\n", img.thumb_offset, img.thumb_length, img.ifd_size, img.exif_data_length);
    return 0;
}

