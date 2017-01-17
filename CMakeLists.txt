# CMake 3.4.3 required to export all symbols on windows.
cmake_minimum_required(VERSION 3.4.3)

project(psf)

# organize targets into folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# automatically export all DLL symbols on windows.
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# set build configuration for make
set(CMAKE_BUILD_TYPE Release)

# enable C++11
set(CMAKE_CXX_STANDARD 11)

# set platform dependent compilation options
if (UNIX)
  message(status "** Setting GCC flags")
  # -fexceptions : enable exception handling
  # -g : include symbol information in executable for debugging.
  # -Wall : enable all warnings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -g -Wall -Wno-unused-variable")
else()
  message(status "** Setting MSVC flags")
  # s flag: enable C++ exceptions handling
  # c flag: functions declared as extern "C" will never throw C++ exception.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()

message(status "** CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# find HDF5 library, require C++ high level API library.
# you can set HDF5_ROOT environment variable to help CMake find HDF5.
find_package(HDF5 REQUIRED COMPONENTS CXX)

message(status "** HDF5 Include: ${HDF5_INCLUDE_DIRS}")
message(status "** HDF5 Libraries Directories: ${HDF5_LIBRARY_DIRS}")
message(status "** HDF5 Libraries: ${HDF5_CXX_LIBRARIES}")

# link HDF5 dynamically
add_definitions(-DH5_BUILT_AS_DYNAMIC_LIB)

# find boost library.
# BOOST_ROOT and BOOST_LIBRARYDIR environment variables
# should be set to help cmake find BOOST location,
# and BOOST_LIBRARYDIR should be added to PATH on windows
# (LD_LIBRARY_PATH for linux) for the program to find the DLL.
# find_package(Boost 1.60.0 REQUIRED COMPONENTS system date_time)

# message(status "** Boost Include: ${Boost_INCLUDE_DIR}")
# message(status "** Boost Libraries: ${Boost_LIBRARY_DIRS}")
# message(status "** Boost Libraries: ${Boost_LIBRARIES}")

# make sure boost libraries are linked dynamically.
#  set(Boost_USE_STATIC_LIBS        OFF)
# set(Boost_USE_MULTITHREADED      ON)
# set(Boost_USE_STATIC_RUNTIME     OFF)
# set(BOOST_ALL_DYN_LINK           ON) 

# disable default log file folder for EASYLOGGING++
add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)

# add subdirectories
add_subdirectory(src lib)
add_subdirectory(test bin)
