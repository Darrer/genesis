#include "ROM.h"

#include <iostream>


void print_usage(char* prog_path)
{
    std::cout << "Usage ./" << prog_path << " <path to rom>" << std::endl;
}

int main(int args, char* argv[])
{
    if(args != 2)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    genesis::ROM rom(argv[1]);
    genesis::print_rom_header(std::cout, rom.header());

    return 0;
}
