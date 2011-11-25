cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake  ${CMAKE_SOURCE_DIR}/deps/gadgetron/cmake)

find_package(Gadgetron REQUIRED)
find_package(Boost REQUIRED)
find_package(ACE REQUIRED)
find_package(TinyXML REQUIRED)

set(CMAKE_INSTALL_PREFIX ${GADGETRON_HOME}/gadgetron)

MESSAGE("GADGETRON_INCLUDE_DIR, ${GADGETRON_INCLUDE_DIR}.")

INCLUDE_DIRECTORIES(${GADGETRON_INCLUDE_DIR} ${ACE_INCLUDE_DIR})
LINK_DIRECTORIES(${GADGETRON_LIB_DIR})

add_subdirectory(siemens_mriclient)
