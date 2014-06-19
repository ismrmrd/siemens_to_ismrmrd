cmake_minimum_required(VERSION 2.6)

project(SIEMENS_TO_ISMRMRD)

#Set the build type to Release if not specified                                                                                                                                                                                                                                         
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

if (WIN32)
    ADD_DEFINITIONS(-DWIN32 -D_WIN32 -D_WINDOWS)
    ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /MP")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    SET (CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
    SET (CMAKE_SHARED_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
    SET (CMAKE_STATIC_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
    SET (CMAKE_MODULE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
endif (WIN32)

###############################################################
#Bootstrap search for libraries (We need to find cmake modules in Gadgetron)
###############################################################
find_path(GADGETRON_CMAKE_MODULES FindGadgetron.cmake HINTS
  $ENV{GADGETRON_HOME}/cmake
  /usr/local/gadgetron/cmake)

if (NOT GADGETRON_CMAKE_MODULES)
   #TODO: Check if path found otherwise create error
   MESSAGE(FATAL_ERROR "GADGETRON_CMAKE_MODULES cannot be found. 
   Try to set GADGETRON_HOME environment variable.")
endif(NOT GADGETRON_CMAKE_MODULES)

set(CMAKE_MODULE_PATH ${GADGETRON_CMAKE_MODULES})
###############################################################


if(WIN32)
    set(Boost_NO_BOOST_CMAKE ON)
endif(WIN32)

find_package(Gadgetron REQUIRED)
find_package(Boost COMPONENTS thread system program_options REQUIRED)
find_package(ACE REQUIRED)
find_package(HDF5 1.8 REQUIRED C CXX)
find_package(Ismrmrd REQUIRED)
find_package(XSD REQUIRED)
find_package(XercesC REQUIRED)


#On windows we will call xsltproc via a system call and the libraries are not needed for compile.
IF (NOT WIN32)
    find_package(LibXml2 REQUIRED)
    find_package(LibXslt REQUIRED)
    INCLUDE_DIRECTORIES(${GADGETRON_INCLUDE_DIR} ${ACE_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR} ${LIBXSLT_INCLUDE_DIR})
ELSE(NOT WIN32)
    INCLUDE_DIRECTORIES(${GADGETRON_INCLUDE_DIR} ${ACE_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
ENDIF(NOT WIN32)

set(CMAKE_INSTALL_PREFIX ${GADGETRON_HOME})
 
LINK_DIRECTORIES(${GADGETRON_LIB_DIR})

add_subdirectory(dependencies)

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

set(schema_files parameter_maps/IsmrmrdParameterMap.xml
               parameter_maps/IsmrmrdParameterMap.xsl
               parameter_maps/IsmrmrdParameterMap_Siemens.xml
               parameter_maps/IsmrmrdParameterMap_Siemens.xsl
               parameter_maps/IsmrmrdParameterMap_Siemens_VB17.xml
               parameter_maps/IsmrmrdParameterMap_Siemens_EPI.xsl
               parameter_maps/IsmrmrdParameterMap_Siemens_EPI_FLASHREF.xsl
               )

add_executable(siemens_to_ismrmrd 
               main.cpp 
               siemensraw.cpp 
               XNode.cpp 
               XNodeParser.cpp 
               vds.cpp
	           defaults.cpp 
               ${ISMRMRD_XSD_SOURCE} 
               siemens_hdf5_datatypes.h 
               siemensraw.h 
               ${schema_files} 
               )

IF (WIN32)
    target_link_libraries(siemens_to_ismrmrd optimized ${HDF5_hdf5_LIBRARY_RELEASE} ${HDF5_hdf5_cpp_LIBRARY_RELEASE})
    target_link_libraries(siemens_to_ismrmrd debug ${HDF5_hdf5_LIBRARY_DEBUG} ${HDF5_hdf5_cpp_LIBRARY_DEBUG})

    set(CMAKE_DEBUG_SUFFIX d CACHE STRING "the debug suffix")
ELSE(WIN32)
    target_link_libraries(siemens_to_ismrmrd tinyxml ${LIBXSLT_LIBRARIES} ${LIBXML2_LIBRARIES} ${HDF5_LIBRARIES})
ENDIF(WIN32)

target_link_libraries(siemens_to_ismrmrd
              optimized cpucore debug cpucore${CMAKE_DEBUG_SUFFIX}
              optimized gadgettools debug gadgettools${CMAKE_DEBUG_SUFFIX}
              optimized gadgetron_mricore debug gadgetron_mricore${CMAKE_DEBUG_SUFFIX}
              gtplus_io
              tinyxml
              ${ISMRMRD_LIBRARIES}
              optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY} 
              ${Boost_LIBRARIES} ${ISMRMRD_LIBRARIES} ${XERCESC_LIBRARIES} )

install(TARGETS siemens_to_ismrmrd DESTINATION bin)
install(FILES ${schema_files} DESTINATION schema)