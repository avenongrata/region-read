#ifndef COMMON_H
#define COMMON_H

//-----------------------------------------------------------------------------

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <iostream>

//-----------------------------------------------------------------------------

#define INPUT_ARGUMENTS_MAX 3

//-----------------------------------------------------------------------------

#define printerr(fmt,...) \
    do {\
            fprintf(stderr, fmt, ## __VA_ARGS__); \
            fflush(stderr); \
    } while(0)

//-----------------------------------------------------------------------------

struct addrs
{
    char ** addrs;
    unsigned int addr_count;
};

struct args
{
    struct addrs addrs;
    unsigned int reg_count;
};

//-----------------------------------------------------------------------------

void read_region(uint32_t start, int size)
{
    unsigned int pagesize = (unsigned)getpagesize();
    unsigned int map_size = pagesize;
    void *map_base, *virt_addr;
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

void show_region(struct args args)
{
    uint32_t target;
    char * endp = NULL;

    for (unsigned int i = 0; i < args.addrs.addr_count; i++)
    {
        target = strtoull(args.addrs.addrs[i], &endp, 0);
        if (errno != 0 || (endp && 0 != *endp))
        {
            printerr("Invalid memory address: %s\n", args.addrs.addrs[i]);
            exit(1);
        }

        read_region(target, args.reg_count);

        printf("\n===============================================\n\n");
    }
}

//-----------------------------------------------------------------------------

void help(void)
{
    printf(
                " Region read v. 1.2                                \n\n"
                " Arguments:                                        \n"
                "    -a         Array of base addresses             \n"
                "    -c         Count of registers (4 byte each)    \n"
                "    -h         Show help message and quit          \n\n");
            exit(EXIT_SUCCESS);
}

//-----------------------------------------------------------------------------

int get_args(int argc, char ** argv)
{
    const char * input_arg[INPUT_ARGUMENTS_MAX] =
    {
        "-a", "-c", "-h"
    };

    unsigned int arg_bit = 0;

    //------------------------------------------------------------------------

    for (int i = 1; i < argc; i++)
    {
        for (int j = 0; j < INPUT_ARGUMENTS_MAX; j++)
        {
            if (!strcmp(argv[i], input_arg[j]))
            {
                if (j == 0)         /* argument -a */
                {
                    arg_bit |= 1;
                }
                else if (j == 1)    /* argument -c */
                {
                    arg_bit |= 2;
                }
                else if (j == 2)    /* argument -h */
                {
                    arg_bit = 425;
                    break;
                }
            }
        }
    }

    //------------------------------------------------------------------------

    if (arg_bit != 425)
    {
        if (arg_bit != 3)
        {
            help();
            exit(1);
        }
    }

    return arg_bit;
}

//----------------------------------------------------------------------------

unsigned int get_reg_count(int argc, char ** argv)
{
    unsigned int reg_count = 0;

    for (int i = 1; i < argc; i++)
    {
        for (int j = 0; j < INPUT_ARGUMENTS_MAX; j++)
        {
            if (!strcmp(argv[i], "-c"))
            {
                ++i;
                if (i == argc)
                {
                    fprintf(stderr, "No register count is specified\n"
                                    "Use argument -h for full details\n\n");
                    exit(EXIT_FAILURE);
                }

                reg_count = atoi(argv[i]);

                if (!reg_count)
                {
                    std::cout << "Register count is 0. "
                              << "Specify please register count."
                              << std::endl;
                    exit(1);
                }

                return reg_count;
            }
        }
    }

    return reg_count;
}

//-----------------------------------------------------------------------------

struct addrs get_addrs(int argc, char ** argv)
{
    struct addrs addrs;
    unsigned int addrs_count = 0;

    //-------------------------------------------------------------------------

    for (int i = 1; i < argc; i++)
    {
        for (int j = 0; j < INPUT_ARGUMENTS_MAX; j++)
        {
            if (!strcmp(argv[i], "-a"))
            {
                ++i;
                if (i == argc)
                {
                    fprintf(stderr, "No base addresses are specified\n"
                                    "Use argument -h for full details\n\n");
                    exit(EXIT_FAILURE);
                }

                // get addrs
                for (int k = i; k < argc; k++)
                {
                    if (!strcmp(argv[k], "-c"))
                    {
                        if (!addrs_count)
                        {
                            fprintf(stderr, "No base addresses are specified\n"
                                            "Use argument -h for full details\n\n");
                            exit(EXIT_FAILURE);
                        }
                        else
                        {
                            // first addr
                            addrs.addrs = &argv[i];
                            // the hole count of addrs
                            addrs.addr_count = addrs_count;

                            return addrs;
                        }
                    }

                    addrs_count++;
                }

                // first addr
                addrs.addrs = &argv[i];
                // the hole count of addrs
                addrs.addr_count = addrs_count;

                return addrs;

            }
        }
    }

    return addrs;
}

//-----------------------------------------------------------------------------

struct args parse_args(int argc, char ** argv, unsigned int arg_bit)
{
    struct args args;

    //-------------------------------------------------------------------------

    // show help message and quit
    if (arg_bit == 425)
    {
        help();
        exit(0);
    }

    //-------------------------------------------------------------------------

    // argument -a
    if (arg_bit & 1)
        args.addrs = get_addrs(argc, argv);

    //-------------------------------------------------------------------------

    // argument -c
    if (arg_bit & 2)
        args.reg_count = get_reg_count(argc, argv);


    //-------------------------------------------------------------------------

    return args;
}

//-----------------------------------------------------------------------------

#endif // COMMON_H
