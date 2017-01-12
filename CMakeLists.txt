cmake_minimum_required(VERSION 3.1)
project (psf)
set(CMAKE_BUILD_TYPE Release)

include_directories("include")

file(GLOB SOURCES "src/*.cpp")

set(CMAKE_CXX_STANDARD 11)

set(LIBRARY_OUTPUT_PATH "build")

add_library("psf" SHARED ${SOURCES})

add_executable("testpsf" "test/test.cpp")

target_link_libraries("testpsf" "boost_system" "psf")
