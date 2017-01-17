
# link HDF5 dynamically
add_definitions(-DH5_BUILT_AS_DYNAMIC_LIB)
# disable default log file folder for EASYLOGGING++
add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)

# make sure boost libraries are linked dynamically.
#  set(Boost_USE_STATIC_LIBS        OFF)
# set(Boost_USE_MULTITHREADED      ON)
# set(Boost_USE_STATIC_RUNTIME     OFF)
# set(BOOST_ALL_DYN_LINK           ON) 

# find boost library.
# BOOST_ROOT and BOOST_LIBRARYDIR environment variables
# should be set to help cmake find BOOST location,
# and BOOST_LIBRARYDIR should be added to PATH on windows
# (LD_LIBRARY_PATH for linux) for the program to find the DLL.
# find_package(Boost 1.60.0 REQUIRED COMPONENTS system date_time)

# message(status "** Boost Include: ${Boost_INCLUDE_DIR}")
# message(status "** Boost Libraries: ${Boost_LIBRARY_DIRS}")
# message(status "** Boost Libraries: ${Boost_LIBRARIES}")

# find HDF5 library, require C++ high level API library.
# you can set HDF5_ROOT environment variable to help CMake find HDF5.
find_package(HDF5 REQUIRED COMPONENTS CXX)

message(status "** HDF5 Include: ${HDF5_INCLUDE_DIRS}")
message(status "** HDF5 Libraries Directories: ${HDF5_LIBRARY_DIRS}")
message(status "** HDF5 Libraries: ${HDF5_CXX_LIBRARIES}")

# list sources explicitly, so if we add/remove sources cmake 
# knows to update makefiles.
set(SOURCES 
    ${CMAKE_SOURCE_DIR}/include/psf.hpp
    psf.cpp
    ${CMAKE_SOURCE_DIR}/include/psftypes.hpp
    psftypes.cpp
    ${CMAKE_SOURCE_DIR}/include/psfproperty.hpp
    psfproperty.cpp
    ${CMAKE_SOURCE_DIR}/include/psfcommon.hpp
    ${CMAKE_SOURCE_DIR}/easyloggingpp/src/easylogging++.h
    ${CMAKE_SOURCE_DIR}/easyloggingpp/src/easylogging++.cc
    )

# setup include directories
include_directories(${CMAKE_SOURCE_DIR}/include
                    ${HDF5_INCLUDE_DIRS}
                    # ${Boost_INCLUDE_DIR}
		    ${CMAKE_SOURCE_DIR}/easyloggingpp/src
                    )

# setup link libraries.
link_directories(${HDF5_LIBRARY_DIRS}
                 # ${Boost_LIBRARY_DIRS} 
                 )

# build shared library
add_library(psf SHARED ${SOURCES})

# shared library dependencies
target_link_libraries(psf
                      ${HDF5_CXX_LIBRARIES}
                      # ${Boost_LIBRARIES}
                      )

# set shared library file folder
set_property(TARGET psf PROPERTY FOLDER "libraries")

# install targets to folders
install(TARGETS psf
        RUNTIME DESTINATION ${psf_BINARY_DIR}/bin
        LIBRARY DESTINATION ${psf_BINARY_DIR}/lib
)
