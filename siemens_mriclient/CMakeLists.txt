add_executable(siemens_mriclient main.cpp siemensraw.cpp XNode.cpp XNodeParser.cpp)

target_link_libraries(siemens_mriclient 
		      optimized gadgettools debug gadgettools${CMAKE_DEBUG_SUFFIX}
		      tinyxml
		      hdf5mri
		      hdf5utils
		      ${HDF5_LIBRARIES} 
		      optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY})

install(TARGETS siemens_mriclient DESTINATION bin)
