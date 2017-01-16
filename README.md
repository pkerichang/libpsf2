Convert spectre Parameter Storage Format (PSF) simulation files to HDF5.

This project is based and rewritten from:

https://github.com/henjo/libpsf

External dependencies are easyloggingpp (as a submodule) and HDF5 C++ API.
This project is built with CMake so if you install HDF5 properly, CMake
should be able to find it.  Note that as long as Anaconda python is in
your path and HDF5/h5py is installed in your Anaconda distribution, CMake
should be able to find HDF5 associated with that and no additional
installation is required.

