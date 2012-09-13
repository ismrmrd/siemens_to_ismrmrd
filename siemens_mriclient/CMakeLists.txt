find_package(Ismrmrd REQUIRED)
find_package(HDF5 1.8 COMPONENTS C CXX REQUIRED)
find_package(XSD REQUIRED)

INCLUDE_DIRECTORIES(${ISMRMRD_INCLUDE_DIR} ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS}/cpp ${XSD_INCLUDE_DIR})
LINK_DIRECTORIES(${BOOST_ROOT}/lib)

add_executable(siemens_mriclient main.cpp siemensraw.cpp XNode.cpp XNodeParser.cpp vds.cpp)
add_executable(siemens_to_HDF5 siemens_to_HDF5.cpp)

IF (WIN32)
	SET(HDF5_LIB_DIR ${HDF5_INCLUDE_DIR}/../lib)
	target_link_libraries(siemens_mriclient optimized ${HDF5_LIB_DIR}/hdf5dll.lib)
	target_link_libraries(siemens_mriclient optimized ${HDF5_LIB_DIR}/hdf5_cppdll.lib)
	target_link_libraries(siemens_mriclient optimized ${HDF5_LIB_DIR}/hdf5_hldll.lib)

	target_link_libraries(siemens_mriclient debug ${HDF5_LIB_DIR}/hdf5ddll.lib)
	target_link_libraries(siemens_mriclient debug ${HDF5_LIB_DIR}/hdf5_cppddll.lib)
	target_link_libraries(siemens_mriclient debug ${HDF5_LIB_DIR}/hdf5_hlddll.lib)

	target_link_libraries(siemens_mriclient optimized ${HDF5_LIB_DIR}/zlib.lib)
	target_link_libraries(siemens_mriclient optimized ${HDF5_LIB_DIR}/szip.lib)
	target_link_libraries(siemens_mriclient debug ${HDF5_LIB_DIR}/zlib_D.lib)
	target_link_libraries(siemens_mriclient debug ${HDF5_LIB_DIR}/szip_D.lib)

	target_link_libraries(siemens_to_HDF5 optimized ${HDF5_LIB_DIR}/hdf5dll.lib)
	target_link_libraries(siemens_to_HDF5 optimized ${HDF5_LIB_DIR}/hdf5_cppdll.lib)
	target_link_libraries(siemens_to_HDF5 optimized ${HDF5_LIB_DIR}/hdf5_hldll.lib)

	target_link_libraries(siemens_to_HDF5 debug ${HDF5_LIB_DIR}/hdf5ddll.lib)
	target_link_libraries(siemens_to_HDF5 debug ${HDF5_LIB_DIR}/hdf5_cppddll.lib)
	target_link_libraries(siemens_to_HDF5 debug ${HDF5_LIB_DIR}/hdf5_hlddll.lib)

	target_link_libraries(siemens_to_HDF5 optimized ${HDF5_LIB_DIR}/zlib.lib)
	target_link_libraries(siemens_to_HDF5 optimized ${HDF5_LIB_DIR}/szip.lib)
	target_link_libraries(siemens_to_HDF5 debug ${HDF5_LIB_DIR}/zlib_D.lib)
	target_link_libraries(siemens_to_HDF5 debug ${HDF5_LIB_DIR}/szip_D.lib)

ELSE(WIN32)	
	target_link_libraries(siemens_mriclient ${LIBXSLT_LIBRARIES} ${LIBXML2_LIBRARIES} ${HDF5_LIBRARIES})	
	target_link_libraries(siemens_to_HDF5 ${HDF5_LIBRARIES})
ENDIF(WIN32)

target_link_libraries(siemens_mriclient
		      optimized gadgettools debug gadgettools${CMAKE_DEBUG_SUFFIX}
		      tinyxml
		      ${ISMRMRD_LIBRARIES}
		      optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY} ${Boost_LIBRARIES})

target_link_libraries(siemens_to_HDF5 
		      optimized gadgettools debug gadgettools${CMAKE_DEBUG_SUFFIX}
		      tinyxml
		      optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY} ${Boost_LIBRARIES})

install(TARGETS siemens_mriclient siemens_to_HDF5 DESTINATION bin)
install(FILES ${ISMRMRD_LIBRARIES} DESTINATION lib)
install(FILES ${ISMRMRD_SCHEMA_DIR}/ismrmrd.xsd DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap.xml DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap.xsl DESTINATION schema)
