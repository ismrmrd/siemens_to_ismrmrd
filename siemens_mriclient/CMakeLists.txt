find_package(Ismrmrd REQUIRED)
find_package(HDF5 1.8 COMPONENTS C CXX REQUIRED)
find_package(XSD REQUIRED)
find_package(XercesC REQUIRED)

# whether to compile toolboxes as static library
OPTION(BUILD_TOOLBOX_STATIC "Build static library for toolboxes" Off)

if ( BUILD_TOOLBOX_STATIC )
    message("Use static toolbox libray ... ")
    ADD_DEFINITIONS(-DBUILD_TOOLBOX_STATIC)
else ( BUILD_TOOLBOX_STATIC )
    message("Use dynamic toolbox libray ... ")
endif ( BUILD_TOOLBOX_STATIC )

if(WIN32)
  link_directories(${Boost_LIBRARY_DIRS})
endif(WIN32)

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/dependencies/tinyxml
  ${ISMRMRD_INCLUDE_DIR} ${ISMRMRD_SCHEMA_DIR} ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS}/cpp ${XSD_INCLUDE_DIR})

IF (WIN32)
    link_libraries(optimized ${HDF5_hdf5_LIBRARY_RELEASE} optimized ${HDF5_hdf5_cpp_LIBRARY_RELEASE} optimized ${HDF5_hdf5_hl_LIBRARY_RELEASE} 
	                 debug ${HDF5_hdf5_LIBRARY_DEBUG} debug ${HDF5_hdf5_cpp_LIBRARY_DEBUG} debug ${HDF5_hdf5_hl_LIBRARY_DEBUG})
    set(CMAKE_DEBUG_SUFFIX d CACHE STRING "the debug suffix")
	add_executable(siemens_mriclient main.cpp siemensraw.cpp XNode.cpp XNodeParser.cpp vds.cpp ${ISMRMRD_XSD_SOURCE})
    add_executable(siemens_to_HDF5 siemens_to_HDF5.cpp ${ISMRMRD_XSD_SOURCE})
ELSE(WIN32)    
    add_executable(siemens_mriclient main.cpp siemensraw.cpp XNode.cpp XNodeParser.cpp vds.cpp ${ISMRMRD_XSD_SOURCE})
    add_executable(siemens_to_HDF5 siemens_to_HDF5.cpp ${ISMRMRD_XSD_SOURCE})
    target_link_libraries(siemens_mriclient tinyxml ${LIBXSLT_LIBRARIES} ${LIBXML2_LIBRARIES} ${HDF5_LIBRARIES})    
    target_link_libraries(siemens_to_HDF5 tinyxml ${HDF5_LIBRARIES})
ENDIF(WIN32)

target_link_libraries(siemens_mriclient
              optimized cpucore debug cpucore${CMAKE_DEBUG_SUFFIX}
              optimized gadgettools debug gadgettools${CMAKE_DEBUG_SUFFIX}
              optimized gadgetron_mricore debug gadgetron_mricore${CMAKE_DEBUG_SUFFIX}
              #optimized tinyxml debug tinyxml${CMAKE_DEBUG_SUFFIX}
              tinyxml
              ${ISMRMRD_LIBRARIES}
              optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY} 
              ${Boost_LIBRARIES} ${ISMRMRD_LIBRARIES} ${XERCESC_LIBRARIES} )

target_link_libraries(siemens_to_HDF5 
              optimized cpucore debug cpucore${CMAKE_DEBUG_SUFFIX}
              optimized gadgettools debug gadgettools${CMAKE_DEBUG_SUFFIX}
              optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY} 
              ${Boost_LIBRARIES} ${ISMRMRD_LIBRARIES} ${XERCESC_LIBRARIES} 
              )

install(TARGETS siemens_mriclient siemens_to_HDF5 DESTINATION bin)
#install(FILES ${ISMRMRD_LIBRARIES} DESTINATION lib)
#install(FILES ${ISMRMRD_SCHEMA_DIR}/ismrmrd.xsd DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap.xml DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap.xsl DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap_Siemens.xml DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap_Siemens.xsl DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap_Siemens_VB17.xml DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap_Siemens_EPI.xml DESTINATION schema)
install(FILES parameter_maps/IsmrmrdParameterMap_Siemens_EPI.xsl DESTINATION schema)
