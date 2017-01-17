# list sources explicitly, so if we add/remove sources cmake 
# knows to update makefiles.
set(SOURCES
    test.cpp
    )
  
# build test executable
add_executable(testpsf ${SOURCES})

# setup include directories
include_directories(${CMAKE_SOURCE_DIR}/include
                    ${HDF5_INCLUDE_DIRS}
                    # ${Boost_INCLUDE_DIR}
		    ${CMAKE_SOURCE_DIR}/easyloggingpp/src
                    )

# test executable dependencies (transitive)
target_link_libraries(testpsf 
                      psf
                      )

# set test executable folder
set_property(TARGET testpsf PROPERTY FOLDER "executables")

# install targets to folders
install(TARGETS testpsf  
        RUNTIME DESTINATION ${psf_BINARY_DIR}/bin
        LIBRARY DESTINATION ${psf_BINARY_DIR}/bin
)
