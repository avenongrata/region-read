#include <iostream>
#include "common.h"

//-----------------------------------------------------------------------------

int main(int argc, char ** argv)
{
    unsigned int arg_bit = 0;
    struct args args;

    // incorrect using of program
    if (argc < 5)
    {
        help();
        exit(0);
    }

    //-------------------------------------------------------------------------

    arg_bit = get_args(argc, argv);
    args = parse_args(argc, argv, arg_bit);

    //-------------------------------------------------------------------------

    show_region(args);

    return 0;
}