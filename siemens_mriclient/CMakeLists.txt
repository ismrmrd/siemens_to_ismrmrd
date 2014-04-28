find_package(Ismrmrd REQUIRED)
find_package(HDF5 1.8 COMPONENTS C CXX REQUIRED)
find_package(XSD REQUIRED)
find_package(XercesC REQUIRED)

#find_package(MKL)

#find_package(CUDA 4.1)
#if (CUDA_FOUND)
#    ADD_DEFINITIONS(-DUSE_CUDA)
#    find_package(CULA)
#endif (CUDA_FOUND)

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

INCLUDE_DIRECTORIES( ${ISMRMRD_INCLUDE_DIR}
                     ${ISMRMRD_SCHEMA_DIR} 
                     ${HDF5_CXX_INCLUDE_DIR} 
                     ${HDF5_C_INCLUDE_DIR} 
                     ${XSD_INCLUDE_DIR} 
                     ${CMAKE_SOURCE_DIR}/dependencies/tinyxml )

#Process the XSD files
SET(XSDS schema/IceGadgetron.xsd)
SET(XSD_ARGS cxx-tree --generate-serialization)
WRAP_XSD(XSDS_SOURCES XSD_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/schema ${XSDS} OPTIONS ${XSD_ARGS})
INCLUDE_DIRECTORIES(${XSD_INCLUDES} ${XERCESC_INCLUDE_DIR})

message("XSDS_SOURCES is " ${XSDS_SOURCES})

set(schema_files parameter_maps/IsmrmrdParameterMap.xml
               parameter_maps/IsmrmrdParameterMap.xsl
               parameter_maps/IsmrmrdParameterMap_Siemens.xml
               parameter_maps/IsmrmrdParameterMap_Siemens.xsl
               parameter_maps/IsmrmrdParameterMap_Siemens_VB17.xml
               parameter_maps/IsmrmrdParameterMap_Siemens_EPI.xml
               parameter_maps/IsmrmrdParameterMap_Siemens_EPI.xsl
               parameter_maps/IsmrmrdParameterMap_Siemens_EPI_FLASHREF.xsl
               )

add_executable(siemens_mriclient 
               main.cpp 
               siemensraw.cpp 
               XNode.cpp 
               XNodeParser.cpp 
               vds.cpp 
               ${ISMRMRD_XSD_SOURCE} 
               siemens_hdf5_datatypes.h 
               siemensraw.h 
               ${schema_files} 
               )

add_executable(siemens_to_HDF5 siemens_to_HDF5.cpp ${ISMRMRD_XSD_SOURCE})

add_executable(siemens_IceGadgetron siemens_IceGadgetron.cpp IceGadgetronConfig.h ${XSDS_SOURCES} ${XSD_INCLUDES} schema/IceGadgetron.xsd)

IF (WIN32)
    target_link_libraries(siemens_mriclient optimized ${HDF5_hdf5_LIBRARY_RELEASE} ${HDF5_hdf5_cpp_LIBRARY_RELEASE})
    target_link_libraries(siemens_mriclient debug ${HDF5_hdf5_LIBRARY_DEBUG} ${HDF5_hdf5_cpp_LIBRARY_DEBUG})

    target_link_libraries(siemens_to_HDF5 optimized ${HDF5_hdf5_LIBRARY_RELEASE} ${HDF5_hdf5_cpp_LIBRARY_RELEASE})
    target_link_libraries(siemens_to_HDF5 debug ${HDF5_hdf5_LIBRARY_DEBUG} ${HDF5_hdf5_cpp_LIBRARY_DEBUG})

#    IF (MKL_FOUND)
#        target_link_libraries(siemens_mriclient ${MKL_LIBRARIES})
#    ENDIF (MKL_FOUND)

    set(CMAKE_DEBUG_SUFFIX d CACHE STRING "the debug suffix")
ELSE(WIN32)
#    IF (MKL_FOUND)
#        target_link_libraries(siemens_mriclient tinyxml ${LIBXSLT_LIBRARIES} ${LIBXML2_LIBRARIES} ${HDF5_LIBRARIES} ${MKL_LIBRARIES})
#    ELSE (MKL_FOUND)
        target_link_libraries(siemens_mriclient tinyxml ${LIBXSLT_LIBRARIES} ${LIBXML2_LIBRARIES} ${HDF5_LIBRARIES})
#    ENDIF (MKL_FOUND)

    target_link_libraries(siemens_to_HDF5 tinyxml ${HDF5_LIBRARIES})
ENDIF(WIN32)

#IF (CUDA_FOUND)
#    target_link_libraries(siemens_mriclient ${CUDA_LIBRARIES} ${CUDA_CUFFT_LIBRARIES} ${CUDA_CUBLAS_LIBRARIES} ${CULA_LIBRARIES})
#    IF (CULA_FOUND)
#        target_link_libraries(siemens_mriclient ${CULA_LIBRARIES})
#    ENDIF(CULA_FOUND)
#ENDIF (CUDA_FOUND)

target_link_libraries(siemens_mriclient
              optimized cpucore debug cpucore${CMAKE_DEBUG_SUFFIX}
              optimized cpucore_math debug cpucore_math${CMAKE_DEBUG_SUFFIX}
              optimized gadgettools debug gadgettools${CMAKE_DEBUG_SUFFIX}
              optimized gadgetron_mricore debug gadgetron_mricore${CMAKE_DEBUG_SUFFIX}
              gtplus_io
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

target_link_libraries(siemens_IceGadgetron 
              tinyxml
              optimized cpucore debug cpucore${CMAKE_DEBUG_SUFFIX}
              optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY} 
              ${Boost_LIBRARIES} ${ISMRMRD_LIBRARIES} ${XERCESC_LIBRARIES} 
              )

install(TARGETS siemens_mriclient siemens_to_HDF5 siemens_IceGadgetron DESTINATION bin)
install(FILES schema/IceGadgetron.xsd DESTINATION schema)
install(FILES ${schema_files} DESTINATION schema)
