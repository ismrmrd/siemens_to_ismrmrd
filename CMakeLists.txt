cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/deps/gadgetron/cmake)

find_package(Boost REQUIRED)
find_package(ACE REQUIRED)
find_package(TinyXML REQUIRED)

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
