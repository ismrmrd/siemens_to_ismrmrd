cmake_minimum_required(VERSION 2.6)
project(GT_SIEMENS_APPS)

if (WIN32)
ADD_DEFINITIONS(-DWIN32 -D_WIN32 -D_WINDOWS)
ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
endif (WIN32)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/deps/gadgetron/cmake)
set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/gadgetron)

find_package(Boost REQUIRED)
find_package(ACE REQUIRED)

if(WIN32)
   SET( GAD_ID gadgetroncore.lib )
   FIND_PATH( GADGETRON_LIB_PATH ${GAD_ID})
else(WIN32)
   SET( GAD_ID gadgetroncore.so )
   FIND_PATH( GADGETRON_LIB_PATH ${GAD_ID}
              PATHS /usr/lib /usr/local/lib )
endif( WIN32 )

MARK_AS_ADVANCED(GADGETRON_LIB_PATH) 
IF (NOT GADGETRON_LIB_PATH) 
	MESSAGE(FATAL_ERROR "Could not find Gadgetron libraries") 
ENDIF (NOT GADGETRON_LIB_PATH) 

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/gadgetron/toolboxes/hostutils/)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/gadgetron/toolboxes/ndarray/)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/gadgetron/toolboxes/solvers/)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/gadgetron/toolboxes/gadgettools/)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/gadgetron/apps/gadgetron/)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/gadgetron/apps/mriclient/)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/gadgetron/gadgets/core/)

LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/gadgetron/build/toolboxes/hostutils/)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/deps/gadgetron/build/toolboxes/gadgettools/)

add_subdirectory(siemens_mriclient)
