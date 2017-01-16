#include <iostream>
#include <fstream>
#include "psf.hpp"
#include "H5Cpp.h"


int main(int argc, char *argv[]) {
    if (argc >= 2) {
        std::string fname = argv[1];
        try {
            psf::read_psf(fname, "test.hdf5", "output.log", true);
        }
        catch (std::exception & e) {
            std::cout << "Exception caught: " << std::endl;
            std::cout << e.what() << std::endl;
        }
    }
    else {
        std::cout << "No file specified, nothing to do." << std::endl;
    }
    /*
    try {
        const std::string file_name("example.hdf5");
        const std::string dn1("foo");
        const std::string dn2("bar");
        auto file = std::unique_ptr<H5::H5File>(new H5::H5File(file_name.c_str(), H5F_ACC_TRUNC));

        hsize_t dims1[2] = { 2, 10 };
        hsize_t dims2[1] = { 10 };
        hsize_t dims3[1] = { 6 };
        double data1[2][10];
        typedef struct comp {
            double r;
            double i;
        } comp;
        comp data2[10];
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 10; k++) {
                data1[j][k] = j * 6 + 2 * k * k;
                data2[k].r = 3.5 * (j + 1);
                data2[k].i = 4.3 * (k * k + 1);
            }
        }
        H5::DataSpace dataspace1(2, dims1);
        H5::DataSpace dataspace2(1, dims2);
        H5::DataSpace dataspace3(1, dims3);

        std::cout << "1" << std::endl;

        auto rname = std::unique_ptr<std::string>(new std::string("real"));
        auto iname = std::unique_ptr<std::string>(new std::string("imag"));


        H5::CompType complex_type(sizeof(comp));
        complex_type.insertMember(*rname, HOFFSET(comp, r), H5::PredType::IEEE_F64LE);
        complex_type.insertMember(*iname, HOFFSET(comp, i), H5::PredType::IEEE_F64LE);

        std::cout << "3" << std::endl;
        std::cout << "rname: " << *rname << std::endl;
        std::cout << "iname: " << *iname << std::endl;
        std::string a = complex_type.getMemberName(0);


        auto dataset1 = std::unique_ptr<H5::DataSet>(new H5::DataSet(file->createDataSet(dn1.c_str(),
            H5::PredType::IEEE_F64LE, dataspace1)));
        auto dataset2 = std::unique_ptr<H5::DataSet>(new H5::DataSet(file->createDataSet(dn2.c_str(),
            complex_type, dataspace2)));
        dataset1->write(data1, H5::PredType::IEEE_F64LE);
        dataset2->write(data2, complex_type, dataspace3, dataspace3);

        dataset1->close();
        dataset2->close();
        file->close();

    } catch (std::exception & e) {
        std::cout << "Exception caught: " << std::endl;
        std::cout << e.what() << std::endl;
    }
    */
    std::cout << "Press enter to continue";
    std::cin.ignore();
    return 1;
}

