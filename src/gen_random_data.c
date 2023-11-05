#include "stdio.h"
#include "stdlib.h"

int main(int argc, char* argv[]) {
    unsigned long total_size;
    char *file_name;

    unsigned char * buffer;
    FILE * fd;

    int ret = -1;
    
    if (argc != 3) {
        printf("*** Error: incorrect usage of %s\n", argv[0]);
        printf("Usage: %s SIZE FILE_NAME\n", argv[0]);
        return -1;
    }

    total_size = strtoul(argv[1], NULL, 10);
    file_name = argv[2];

    buffer = (unsigned char *)malloc(total_size);
    if (buffer == NULL) {
        printf("*** ERROR: failed to allocate memory\n");
        goto close_and_clear;
    }
    fd = fopen(file_name, "wb");
    if (fd < 0) {
        printf("*** ERROR: failed to open target file\n");
        goto close_and_clear;
    }

    for (size_t i = 0; i < total_size; i++) {
        buffer[i] = (unsigned char)(rand()%256);
    }
    fwrite(buffer, 1, total_size, fd);

close_and_clear:
    if (buffer != NULL) free(buffer);
    if (fd >= 0) fclose(fd);
    return ret;
}