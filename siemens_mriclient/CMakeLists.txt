cmake_policy(SET CMP0008 OLD)

include_directories( ${ACE_INCLUDE_DIR} 
                     ${Boost_INCLUDE_DIR}
                     ${CMAKE_SOURCE_DIR}/deps/gadgetron/dependencies/tinyxml
                     ${CMAKE_SOURCE_DIR}/deps/gadgetron/gadgets/core 
		             ${CMAKE_SOURCE_DIR}/deps/gadgetron/apps/gadgetron
		             ${CMAKE_SOURCE_DIR}/deps/gadgetron/apps/mriclient 
		             ${CMAKE_SOURCE_DIR}/deps/gadgetron/toolboxes/ndarray
		             ${CMAKE_SOURCE_DIR}/deps/gadgetron/toolboxes/gadgettools)

add_executable(siemens_mriclient main.cpp siemensraw.cpp XNode.cpp XNodeParser.cpp)

target_link_libraries(siemens_mriclient 
					  ${GADGETRON_LIB_PATH}/gadgettools${CMAKE_DEBUG_POSTFIX} ${GADGETRON_LIB_PATH}/tinyxml${CMAKE_DEBUG_POSTFIX} 
					  optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY})

install(TARGETS siemens_mriclient DESTINATION bin)
