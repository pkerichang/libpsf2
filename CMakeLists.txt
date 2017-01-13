cmake_minimum_required(VERSION 3.1)

add_definitions(-DBOOST_LOG_DYN_LINK)

project (psf)

set(CMAKE_BUILD_TYPE Release)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp)

set(CMAKE_CXX_STANDARD 11)

set(BOOST_ROOT /home/erichang/Applications/anaconda2/include)

find_library(BOOST boost_system)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(psf SHARED ${SOURCES})

target_link_libraries(psf ${BOOST})

add_executable("testpsf" "test/test.cpp")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries("testpsf" "psf" ${BOOST})
