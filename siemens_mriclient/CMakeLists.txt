find_package(Ismrmrd REQUIRED)
find_package(HDF5 1.8 COMPONENTS C CXX REQUIRED)
find_package(XSD REQUIRED)
find_package(XercesC REQUIRED)

if(WIN32)
  link_directories(${Boost_LIBRARY_DIRS})
endif(WIN32)

INCLUDE_DIRECTORIES(${ISMRMRD_INCLUDE_DIR} ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS}/cpp ${XSD_INCLUDE_DIR})

add_executable(siemens_mriclient main.cpp siemensraw.cpp XNode.cpp XNodeParser.cpp vds.cpp ${ISMRMRD_XSD_SOURCE})
add_executable(siemens_to_HDF5 siemens_to_HDF5.cpp ${ISMRMRD_XSD_SOURCE})

IF (WIN32)
	SET(HDF5_LIB_DIR ${HDF5_INCLUDE_DIR}/../lib)
	target_link_libraries(siemens_mriclient optimized ${HDF5_LIB_DIR}/hdf5dll.lib)
	target_link_libraries(siemens_mriclient optimized ${HDF5_LIB_DIR}/hdf5_cppdll.lib)
	target_link_libraries(siemens_mriclient optimized ${HDF5_LIB_DIR}/hdf5_hldll.lib)

	target_link_libraries(siemens_mriclient debug ${HDF5_LIB_DIR}/hdf5ddll.lib)
	target_link_libraries(siemens_mriclient debug ${HDF5_LIB_DIR}/hdf5_cppddll.lib)
	target_link_libraries(siemens_mriclient debug ${HDF5_LIB_DIR}/hdf5_hlddll.lib)

	target_link_libraries(siemens_to_HDF5 optimized ${HDF5_LIB_DIR}/hdf5dll.lib)
	target_link_libraries(siemens_to_HDF5 optimized ${HDF5_LIB_DIR}/hdf5_cppdll.lib)
	target_link_libraries(siemens_to_HDF5 optimized ${HDF5_LIB_DIR}/hdf5_hldll.lib)

	target_link_libraries(siemens_to_HDF5 debug ${HDF5_LIB_DIR}/hdf5ddll.lib)
	target_link_libraries(siemens_to_HDF5 debug ${HDF5_LIB_DIR}/hdf5_cppddll.lib)
	target_link_libraries(siemens_to_HDF5 debug ${HDF5_LIB_DIR}/hdf5_hlddll.lib)

	set(CMAKE_DEBUG_SUFFIX d CACHE STRING "the debug suffix")

ELSE(WIN32)	
	target_link_libraries(siemens_mriclient ${LIBXSLT_LIBRARIES} ${LIBXML2_LIBRARIES} ${HDF5_LIBRARIES})	
	target_link_libraries(siemens_to_HDF5 ${HDF5_LIBRARIES})
ENDIF(WIN32)

target_link_libraries(siemens_mriclient
		      optimized gadgettools debug gadgettools${CMAKE_DEBUG_SUFFIX}
              optimized gadgetroncore debug gadgetroncore${CMAKE_DEBUG_SUFFIX}
		      optimized tinyxml debug tinyxml${CMAKE_DEBUG_SUFFIX} 
		      ${ISMRMRD_LIBRARIES}
		      optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY} 
			  ${Boost_LIBRARIES} ${ISMRMRD_LIBRARIES} ${XERCESC_LIBRARIES} 
			  )

target_link_libraries(siemens_to_HDF5 
		      optimized gadgettools debug gadgettools${CMAKE_DEBUG_SUFFIX}
		      optimized tinyxml debug tinyxml${CMAKE_DEBUG_SUFFIX} 
		      optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY} 
			  ${Boost_LIBRARIES} ${ISMRMRD_LIBRARIES} ${XERCESC_LIBRARIES} 
			  )

install(TARGETS siemens_mriclient siemens_to_HDF5 DESTINATION bin)
install(FILES ${ISMRMRD_LIBRARIES} DESTINATION lib)
install(FILES ${ISMRMRD_SCHEMA_DIR}/ismrmrd.xsd DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap.xml DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap.xsl DESTINATION schema)
