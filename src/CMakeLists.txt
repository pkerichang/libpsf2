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

# build shared library
add_library(psf SHARED ${SOURCES})

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
