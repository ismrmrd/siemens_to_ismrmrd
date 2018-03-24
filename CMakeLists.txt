cmake_minimum_required(VERSION 2.6)

project(SIEMENS-TO-ISMRMRD)

# --- Provide good defaults for searching for packages (i.e. ismrmrd)
set(CMAKE_PREFIX_PATH "/usr")
if(CMAKE_PREFIX_PATH)
  list(APPEND CMAKE_PREFIX_PATH "/usr/local")
endif()
if(EXISTS $ENV{CMAKE_PREFIX_PATH})
  list(APPEND CMAKE_PREFIX_PATH $ENV{CMAKE_PREFIX_PATH})
endif()
if(EXISTS $ENV{ISMRMRD_HOME})
  list(APPEND CMAKE_PREFIX_PATH $ENV{ISMRMRD_HOME})
endif()
list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
message(STATUS "Looking for packages in : ${CMAKE_PREFIX_PATH}")
# ---

#VERSIONING
set(SIEMENS_TO_ISMRMRD_VERSION_MAJOR 1)
set(SIEMENS_TO_ISMRMRD_VERSION_MINOR 0)
set(SIEMENS_TO_ISMRMRD_VERSION_PATCH 1)
set(SIEMENS_TO_ISMRMRD_VERSION_STRING ${SIEMENS_TO_ISMRMRD_VERSION_MAJOR}.${SIEMENS_TO_ISMRMRD_VERSION_MINOR}.${SIEMENS_TO_ISMRMRD_VERSION_PATCH})

# Generate the converter_version.h header file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/converter_version.in ${CMAKE_CURRENT_SOURCE_DIR}/converter_version.h)

# set project specific cmake module path
set (SIEMENS_TO_ISMRMRD_CMAKE_DIR ${PROJECT_SOURCE_DIR}/cmake CACHE PATH
  "Location of CMake scripts")

message ("PROJECT CMAKE DIR: " ${SIEMENS_TO_ISMRMRD_CMAKE_DIR})
message ("PROJECT SOURCE DIR: " ${PROJECT_SOURCE_DIR})

#Set the build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif()

if (WIN32)
    ADD_DEFINITIONS(-DWIN32 -D_WIN32 -D_WINDOWS)
    # ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /MP")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    set (CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
    set (CMAKE_SHARED_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
    set (CMAKE_STATIC_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
    set (CMAKE_MODULE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
    add_definitions(-D__BUILD_CONVERTER_TINYXML__)
else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
endif()

# Added for static linking!!!
if (BUILD_DYNAMIC)
    set(Boost_USE_STATIC_LIBS OFF)
    message("Building dynamically linked application.")
else()
   set(Boost_USE_STATIC_LIBS ON)
   message("Building statically linked application.")
endif()

# set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_NO_BOOST_CMAKE ON)

find_package(Boost COMPONENTS system thread program_options filesystem timer REQUIRED)
find_package(ISMRMRD REQUIRED)
find_package(HDF5 1.8 REQUIRED COMPONENTS C)

include_directories( ${ISMRMRD_INCLUDE_DIR} ${HDF5_C_INCLUDE_DIR} )
link_directories( ${ISMRMRD_LIB_DIR} )

add_executable(embed base64.cpp embed.cpp)

target_link_libraries(embed ${Boost_LIBRARIES})

add_custom_command(
    OUTPUT defaults.cpp
    COMMAND embed ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_VB17.xml 
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens.xml 
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_PreZeros.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_EPI.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_EPI_FLASHREF.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_T1Mapping_SASHA.xsl
                ${ISMRMRD_SCHEMA_DIR}/ismrmrd.xsd
                defaults.cpp
    VERBATIM
    DEPENDS embed ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_VB17.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens.xml 
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_PreZeros.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_EPI.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_EPI_FLASHREF.xsl
                ${CMAKE_CURRENT_SOURCE_DIR}/parameter_maps/IsmrmrdParameterMap_Siemens_T1Mapping_SASHA.xsl
                ${ISMRMRD_SCHEMA_DIR}/ismrmrd.xsd
                )

find_package(LibXml2 REQUIRED)
find_package(LibXslt REQUIRED)

include_directories(${Boost_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR} ${LIBXSLT_INCLUDE_DIR})

if(WIN32)
    link_directories(${Boost_LIBRARY_DIRS})
endif()

add_executable(siemens_to_ismrmrd 
               main.cpp 
               siemensraw.cpp 
               XNode.cpp 
               XNodeParser.cpp 
               vds.cpp
               defaults.cpp
               base64.cpp
               tinyxml.cpp
               tinyxmlerror.cpp
               tinyxmlparser.cpp
               ${schema_files}
               )

target_link_libraries(siemens_to_ismrmrd ${LIBXSLT_LIBRARIES} ${LIBXML2_LIBRARIES})

target_link_libraries(siemens_to_ismrmrd
                        ${ISMRMRD_LIBRARIES}
                        ${Boost_LIBRARIES} )

install(TARGETS siemens_to_ismrmrd DESTINATION bin)

# Create package
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
include(${SIEMENS_TO_ISMRMRD_CMAKE_DIR}/siemens_to_ismrmrd_cpack.cmake)
if(CPACK_GENERATOR)
    message(STATUS "Found CPack generators: ${CPACK_GENERATOR}")
    configure_file("${SIEMENS_TO_ISMRMRD_CMAKE_DIR}/cpack_options.cmake.in" ${SIEMENS_TO_ISMRMRD_CPACK_CFG_FILE} @ONLY)
    set(CPACK_PROJECT_CONFIG_FILE ${SIEMENS_TO_ISMRMRD_CPACK_CFG_FILE})
    include (CPack)
endif()
