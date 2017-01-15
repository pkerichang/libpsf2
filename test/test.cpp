#include <iostream>
#include <fstream>
#include "psf.hpp"


int main(int argc, char *argv[]) {
    if (argc >= 2) {    
        std::string fname = argv[1];
		psf::read_psf(fname);
    } else {
        std::cout << "No file specified, nothing to do." << std::endl;    
    }
	std::cout << "Press enter to continue";
	std::cin.ignore();
}
