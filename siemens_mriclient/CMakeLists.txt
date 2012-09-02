find_package(Ismrmrd REQUIRED)
find_package(HDF5 1.8 COMPONENTS C CXX REQUIRED)

INCLUDE_DIRECTORIES(${ISMRMRD_INCLUDE_DIR})

add_executable(siemens_mriclient main.cpp siemensraw.cpp XNode.cpp XNodeParser.cpp vds.cpp)
add_executable(siemens_to_HDF5 siemens_to_HDF5.cpp)

target_link_libraries(siemens_mriclient
		      optimized gadgettools debug gadgettools${CMAKE_DEBUG_SUFFIX}
		      tinyxml
		      ${ISMRMRD_LIBRARIES}
 			  ${LIBXSLT_LIBRARIES} 
 			  ${LIBXML2_LIBRARIES}
		      ${HDF5_LIBRARIES} 
		      optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY} ${Boost_LIBRARIES})

target_link_libraries(siemens_to_HDF5 
		      optimized gadgettools debug gadgettools${CMAKE_DEBUG_SUFFIX}
		      tinyxml
		      ${HDF5_LIBRARIES} 
		      optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY} ${Boost_LIBRARIES})

install(TARGETS siemens_mriclient siemens_to_HDF5 DESTINATION bin)
install(FILES ${ISMRMRD_LIBRARIES} DESTINATION lib)
install(FILES ${ISMRMRD_SCHEMA_DIR}/ismrmrd.xsd DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap.xml DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap.xsl DESTINATION schema)
