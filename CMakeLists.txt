cmake_minimum_required(VERSION 3.1)

project (psf)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# this variable is needed for CMake to generate static library on windows
# to link wit the executable.
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_BUILD_TYPE Release)

if (UNIX)
  message(status "** Setting GCC flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -g -Wall")
else()
  message(status "** Setting MSVC flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc")
endif()
message(status "** CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# make sure boost libraries are linked directly.
set(Boost_USE_STATIC_LIBS        OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME     OFF)
set(BOOST_ALL_DYN_LINK           ON) 

# list sources explicitly, so if we add/remove sources cmake 
# knows to update makefiles.
set(SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/psf.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/psfcommon.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/psfproperty.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/psftypes.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/psf.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/psfproperty.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/psftypes.cpp
	)

# enable C++11
set(CMAKE_CXX_STANDARD 11)

# find boost library.
# BOOST_ROOT and BOOST_LIBRARYDIR environment variables
# should be set to help cmake find BOOST location,
# and BOOST_LIBRARYDIR should be added to PATH on windows
# (LD_LIBRARY_PATH for linux) for the program to find the DLL.
find_package(Boost 1.60.0 COMPONENTS system date_time REQUIRED)

message(status "** Boost Include: ${Boost_INCLUDE_DIR}")
message(status "** Boost Libraries: ${Boost_LIBRARY_DIRS}")
message(status "** Boost Libraries: ${Boost_LIBRARIES}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${Boost_INCLUDE_DIR})

link_directories(${Boost_LIBRARY_DIRS})

add_library(psf SHARED ${SOURCES})

set_property(TARGET psf PROPERTY FOLDER "libraries")

install(TARGETS psf RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)

add_executable(testpsf test/test.cpp)

target_link_libraries(testpsf psf ${Boost_LIBRARIES})

set_property(TARGET testpsf PROPERTY FOLDER "executables")

install(TARGETS testpsf RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)
