#include <iostream>
#include <fstream>
#include "psf.hpp"


int main(int argc, char *argv[]) {
    if (argc >= 2) {    
        std::string fname = argv[1];
        auto dataset = std::unique_ptr<psf::PSFDataSet>(new psf::PSFDataSet(fname));
        
        auto pdict = dataset->get_prop_dict();
        /*
        std::cout << "header property values: " << std::endl;
        for ( auto it : *pdict) {
            std::cout << it.first << " = " << it.second << std::endl;
        }
        */
        std::cout << std::endl;
    } else {
        std::cout << "No file specified, nothing to do." << std::endl;    
    }
}
