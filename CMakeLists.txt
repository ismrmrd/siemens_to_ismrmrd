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
#Bootstrap search for libraries (We need to find cmake modules in ismrmrd)
###############################################################
find_path(ISMRMRD_CMAKE_MODULES FindIsmrmrd.cmake HINTS
  $ENV{ISMRMRD_HOME}/cmake
  /usr/local/ismrmrd/cmake)

if (NOT ISMRMRD_CMAKE_MODULES)
   #TODO: Check if path found otherwise create error
   MESSAGE(FATAL_ERROR "ISMRMRD_CMAKE_MODULES cannot be found. 
   Try to set ISMRMRD_HOME environment variable.")
endif(NOT ISMRMRD_CMAKE_MODULES)

set(CMAKE_MODULE_PATH ${ISMRMRD_CMAKE_MODULES})
###############################################################

if(WIN32)
    set(Boost_NO_BOOST_CMAKE ON)
endif(WIN32)

find_package(Boost COMPONENTS thread system program_options REQUIRED)
find_package(HDF5 1.8 REQUIRED C CXX)
find_package(Ismrmrd REQUIRED)
find_package(XSD REQUIRED)
find_package(XercesC REQUIRED)

#On windows we will call xsltproc via a system call and the libraries are not needed for compile.
IF (NOT WIN32)
    find_package(LibXml2 REQUIRED)
    find_package(LibXslt REQUIRED)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR} ${LIBXSLT_INCLUDE_DIR})
ELSE(NOT WIN32)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
ENDIF(NOT WIN32)

set(CMAKE_INSTALL_PREFIX ${ISMRMRD_HOME})

add_subdirectory(dependencies)

if(WIN32)
    link_directories(${Boost_LIBRARY_DIRS})
endif(WIN32)

INCLUDE_DIRECTORIES( ${ISMRMRD_INCLUDE_DIR}
                     ${ISMRMRD_SCHEMA_DIR} 
                     ${HDF5_CXX_INCLUDE_DIR} 
                     ${HDF5_C_INCLUDE_DIR} 
                     ${XSD_INCLUDE_DIR} 
                     ${CMAKE_SOURCE_DIR}/dependencies/tinyxml )

add_executable(b64
    base64.cpp
    )

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/defaults.cpp
    COMMAND b64 ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_VB17.xml ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens.xml ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens.xsl ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/ismrmrd.xsd ${CMAKE_CURRENT_SOURCE_DIR}/defaults.cpp
    VERBATIM
    DEPENDS b64
    	${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_VB17.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens.xsl
        ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/ismrmrd.xsd
)

set(schema_files parameter_maps/IsmrmrdParameterMap.xsl
               parameter_maps/IsmrmrdParameterMap_Siemens.xml
               parameter_maps/IsmrmrdParameterMap_Siemens.xsl
               parameter_maps/IsmrmrdParameterMap_Siemens_VB17.xml
               parameter_maps/IsmrmrdParameterMap_Siemens_EPI.xsl
               parameter_maps/IsmrmrdParameterMap_Siemens_EPI_FLASHREF.xsl
               parameter_maps/ismrmrd.xsd
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
              tinyxml
              ${ISMRMRD_LIBRARIES}
              ${Boost_LIBRARIES} ${ISMRMRD_LIBRARIES} ${XERCESC_LIBRARIES} )

install(TARGETS siemens_to_ismrmrd DESTINATION bin)
install(FILES ${schema_files} DESTINATION schema)
