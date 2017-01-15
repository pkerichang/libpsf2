#include <iostream>
#include <fstream>
#include "psf.hpp"
#include "H5Cpp.h"


int main(int argc, char *argv[]) {
    if (argc >= 2) {    
        std::string fname = argv[1];
		try {
			psf::read_psf(fname);
		} catch (std::exception & e) {
			std::cout << "Exception caught: " << std::endl;
			std::cout << e.what() << std::endl;
		}
    } else {
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
		std::complex<double> data2[10];
		for (int j = 0; j < 2; j++) {
			for (int k = 0; k < 10; k++) {
				data1[j][k] = j * 6 + 2 * k * k;
				data2[k] = std::complex<double>(3.5*(j + 1), 4.3*(k*k + 1));
			}
		}
		H5::DataSpace dataspace1(2, dims1);
		H5::DataSpace dataspace2(1, dims2);
		H5::DataSpace dataspace3(1, dims3);

		std::cout << "1" << std::endl;

		H5::CompType complex_type(sizeof(double) * 2);
		std::cout << "2" << std::endl;

		std::string rname("real");
		std::string iname("imag");

		complex_type.insertMember(rname.c_str(), 0, H5::PredType::IEEE_F64LE);
		complex_type.insertMember(iname.c_str(), sizeof(double), H5::PredType::IEEE_F64LE);

		std::cout << "3" << std::endl;

		H5::DataSet dataset1 = file->createDataSet(dn1.c_str(), H5::PredType::IEEE_F64LE, dataspace1);
		H5::DataSet dataset2 = file->createDataSet(dn2.c_str(), complex_type, dataspace2);
		dataset1.write(data1, H5::PredType::IEEE_F64LE);
		dataset2.write(data2, complex_type, dataspace3, dataspace3);

		dataset1.close();
		dataset2.close();
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

