cmake_minimum_required(VERSION 2.6)

project(GT_SIEMENS_APPS)

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

find_package(Gadgetron REQUIRED)
set(Boost_NO_BOOST_CMAKE ON)
find_package(Boost COMPONENTS thread system REQUIRED)
find_package(ACE REQUIRED)
find_package(HDF5 1.8 REQUIRED C CXX)

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

# add_subdirectory(dependencies)
add_subdirectory(siemens_mriclient)
