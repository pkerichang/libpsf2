#include <iostream>
#include <fstream>
#include "psf.hpp"


int main(int argc, char *argv[]) {
    if (argc >= 2) {    
        std::string fname = argv[1];
		try {
			psf::read_psf(fname);
		}
		catch (std::exception& e) {
			std::cout << "Exceptiong caught:" << std::endl;
			std::cout << e.what() << std::endl;
		}
    } else {
        std::cout << "No file specified, nothing to do." << std::endl;    
    }
	std::cout << "Press enter to continue";
	std::cin.ignore();
}
