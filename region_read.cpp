#include <cstdlib>
#include <fstream>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <inttypes.h>

#include <shared_mutex>
#include <mutex>

//-----------------------------------------------------------------------------

#define printerr(fmt,...) \
do {\
        fprintf(stderr, fmt, ## __VA_ARGS__); fflush(stderr); \
   } while(0)

//-----------------------------------------------------------------------------

void read_region(uint32_t start, int size)
{
    unsigned int pagesize = (unsigned)getpagesize();
    unsigned int map_size = pagesize;
    void * map_base, * virt_addr;
    int fd;
    uint32_t offset = (unsigned int)(start & (pagesize-1));
    int i;
    uint32_t tmp_val = 0;
    
    if (!size)
        return;

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1)
    {
        printerr("Error opening /dev/mem (%d) : %s\n", errno, strerror(errno));
        exit(1);
    }

    map_base = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd,
                    start & ~((typeof(start))pagesize-1));

    if (map_base == (void *) -1)
    {
        printerr("Error mapping (%d) : %s\n", errno, strerror(errno));
        exit(1);
    }
       
    for (i = 0; i < size; i++)
    {
        virt_addr = map_base + offset;
        tmp_val = *((uint32_t *) virt_addr);
        printf("0x%08X - 0x%08X\n", start + i * sizeof(uint32_t), tmp_val);
        offset += sizeof(uint32_t);
    }
    
    if (munmap(map_base, map_size) != 0)
    {
        printerr("ERROR munmap (%d) %s\n", errno, strerror(errno));
    }

    close(fd);
}

//-----------------------------------------------------------------------------

int main(int argc, char ** argv)
{
    int region_size = 0;
    uint32_t target;
    char * endp = NULL;
    
    if (argc == 3)
    {
        target = strtoull(argv[1], &endp, 0);
        if (errno != 0 || (endp && 0 != *endp)) 
        {
            printerr("Invalid memory address: %s\n", argv[1]);
            exit(2);
        }

        region_size = atoi(argv[2]);
    }
    else
    {
        printf("Usage: ./region_read base_addr reg_count\n");
        exit(2);
    }
    
    read_region(target, region_size);
}
