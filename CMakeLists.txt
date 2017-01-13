cmake_minimum_required(VERSION 3.1)

add_definitions(-DBOOST_LOG_DYN_LINK)

project (psf)

set(CMAKE_BUILD_TYPE Release)

include_directories("include")

file(GLOB SOURCES "src/*.cpp")

set(CMAKE_CXX_STANDARD 11)

set(BOOST_ROOT /home/erichang/Applications/anaconda2/include)

find_library(BOOST boost_system)

add_library("psf" SHARED ${SOURCES})

add_executable("testpsf" "test/test.cpp")

target_link_libraries("testpsf" PUBLIC "psf" ${BOOST})
