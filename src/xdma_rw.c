
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>

#include <sys/time.h>

// function : dev_read
// description : read data from device to local memory (buffer), (i.e. device-to-host)
// parameter :
//       dev_fd : device instance
//       addr   : source address in the device
//       buffer : buffer base pointer
//       size   : data size
// return:
//       int : 0=success,  -1=failed
int dev_read (int dev_fd, uint64_t addr, void *buffer, uint64_t size) {
    if (addr) {
        if ( addr != lseek(dev_fd, addr, SEEK_SET) )                             // seek
            return -1;                                                           // seek failed
    }
    if ( size != read(dev_fd, buffer, size) )                                    // read device to buffer
        return -1;                                                               // read failed
    return 0;
}

// function : dev_write
// description : write data from local memory (buffer) to device, (i.e. host-to-device)
// parameter :
//       dev_fd : device instance
//       addr   : target address in the device
//       buffer : buffer base pointer
//       size   : data size
// return:
//       int : 0=success,  -1=failed
int dev_write (int dev_fd, uint64_t addr, void *buffer, uint64_t size) {
    if (addr) {
        if ( addr != lseek(dev_fd, addr, SEEK_SET) )                             // seek
            return -1;                                                           // seek failed        
    }

    if ( size != write(dev_fd, buffer, size) )                                   // write device from buffer
        return -1;                                                               // write failed
    return 0;
}

// function : get_millisecond
// description : get time in millisecond
static uint64_t get_millisecond () {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)(tv.tv_usec / 1000);
}

uint64_t getopt_integer(char *optarg)
{
	int rc;
	uint64_t value;

	rc = sscanf(optarg, "0x%lx", &value);
	if (rc <= 0)
		rc = sscanf(optarg, "%lu", &value);
	//printf("sscanf() = %d, value = 0x%lx\n", rc, value);

	return value;
}


#define  DMA_MAX_SIZE   0x10000000UL

char USAGE [] = 
    "Usage: %s [OPTIONS]\n"
    "\n"
    "The options are as follows:\n"
    "   -r/w  Read/Write data from device to file.\n"
    "   -h    Display help information.\n"
    "   -d device_name\n"
    "         Specify the device name (e.g. /dev/xdma0_h2c_0).\n"
    "   -f file_name\n"
    "         Specify the name of file to be read/write.\n"
    "   -s byte_num\n"
    "         Specify the size of one transaction.\n"
    "   -c packet_num\n"
    "         Specify the total number of transactions.\n"
    "   -a address\n"
    "         Specify the start address on the AXI bus(Only for AXI-MM mode)\n"
    "\n"
    " Example:\n"
    "    %s -r -d /dev/xdma0_c2h_0 -f ./data_output.bin -s 1024 -c 4\n"
    "    Read 4KB data from XDMA(AXI-Stream) to data_output.bin through 4 transactions(1KB).\n"
    "\n"
    "    %s -w -d /dev/xdma0_h2c_0 -f ./data_input.bin -s 2048 -c 512\n"
    "    Write 1MB data from data_input.bin to XDMA(AXI-Stream) through 512 transactions(2KB).\n"
    "\n"
    "Return code:\n"
    "    0: read/write operation is successful.\n"
    "   -1: error\n";


int main (int argc, char *argv[]) {
    int   ret = -1;

    uint64_t millisecond;
    char usage_string [1024];
    char mode_string  [8];
    
    int cmd_opt;
    int mode = -1; // 0: read 1: write
    char *dev_name, *file_name;
    uint64_t trans_size = 32;
    uint64_t trans_count = 1;
    uint64_t address = 0;
    uint64_t total_size;
    
    int   dev_fd = -1;
    FILE *file_p = NULL;
    void *buffer = NULL, *temp_buffer;
    
    sprintf(usage_string, USAGE, argv[0], argv[0], argv[0]);
    // parse input options
	while ((cmd_opt=getopt(argc, argv, "rwhd:f:s:c:a:")) != -1) {
		switch (cmd_opt) {
            case 'r':
                mode = 0;
                strcpy(mode_string, "Read");
                break;
            case 'w':
                mode = 1;
                strcpy(mode_string, "Write");
                break;
            case 'd':
                /* device node name */
                dev_name = optarg;
                break;
            case 'f':
                file_name = optarg;
                break;
            case 's':
                /* size in bytes */
                trans_size = getopt_integer(optarg);
                break;
            case 'c':
                /* size in bytes */
                trans_count = getopt_integer(optarg);
                break;
            case 'a':
                /* RAM address on the AXI bus in bytes */
                address = getopt_integer(optarg);
                break;
            case 'h':
                puts(usage_string);
                return 0;
            case '?':
            default:
                printf("Error: incorrect usage of %s\n", argv[0]);
                puts(usage_string);
                return -1;
		}
	}

    // check legality of input options
    if (mode == -1) {
        printf("*** ERROR: The read/write mode is not specified\n");
        puts(usage_string);
        return -1;
    }
    if (trans_size > DMA_MAX_SIZE || trans_size == 0) {
        printf("*** ERROR: DMA size must larger than 0 and NOT be larger than %lu\n", DMA_MAX_SIZE);
        return -1;
    }
    if (trans_count == 0) {
        printf("*** ERROR: the number of transactions can't be zero\n");
        return -1;
    }
    
    // allocate local memory (buffer)
    total_size = trans_size * trans_count;
    buffer = malloc(total_size);
    if (buffer == NULL) {
        printf("*** ERROR: failed to allocate memory buffer\n");
        goto close_and_clear;
    }
    // open target device
    dev_fd = open(dev_name, O_RDWR);
    if (dev_fd < 0) {
        printf("*** ERROR: failed to open device %s\n", dev_name);
        goto close_and_clear;
    }
    // open file for read/write
    file_p = fopen(file_name, mode == 0 ? "rb" : "wb");
    if (file_p == NULL) {
        printf("*** ERROR: failed to open file %s\n", file_name);
        goto close_and_clear;
    }
    
    
    if (mode == 1) { // read data from file to memory (write mode)
        if (total_size != fread(buffer, 1, total_size, file_p)) {
            printf("*** ERROR: failed to read %s\n", file_name);
            goto close_and_clear;
        }
    }

    printf("Start %s XDMA\n", mode_string);
    printf("Device Name: %s\n", dev_name);
    printf("Transaction Size: %lu bytes\n", trans_size);
    printf("Transaction Count: %lu\n", trans_count);
    printf("AXI Address: %lu\n", address);

    millisecond = get_millisecond(); // get start time of DMA operation
    temp_buffer = buffer;
    for (uint64_t i = 0; i < trans_count; i++) {
        if (mode == 1) {
            if (dev_write(dev_fd, address, temp_buffer, trans_size)) {
                printf("*** ERROR: failed to write %ld transaction to %s\n", i, dev_name);
                goto close_and_clear;
            }            
        } else {
            if (dev_read(dev_fd, address, temp_buffer, trans_size)) {
                printf("*** ERROR: failed to read %ld transaction from %s\n", i, dev_name);
                goto close_and_clear;
            }
        }
        temp_buffer += trans_size;
    }
    millisecond = get_millisecond() - millisecond; // get duration of DMA operation

    if (mode == 0) { // write data from memory to file (read mode)
        if (total_size != fwrite(buffer, 1, total_size, file_p)) {
            printf("*** ERROR: failed to write %s\n", file_name);
            goto close_and_clear;
        }
    }

    millisecond = (millisecond > 0) ? millisecond : 1;
    double data_rate = (double)total_size / millisecond;
    printf("Complete %s XDMA\n", dev_name);
    printf("Total Time=%lu ms   Data Rate=%.1lf KBps\n", millisecond, data_rate);
    ret = 0;
    
close_and_clear:
    if (buffer != NULL) free(buffer);
    if (dev_fd >= 0)    close(dev_fd);
    if (file_p != NULL) fclose(file_p);
    return ret;
}



