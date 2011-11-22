include_directories(${ACE_INCLUDE_DIR} 
                    ${TINYXML_INCLUDE_DIR} 
                    ${CMAKE_SOURCE_DIR}/gadgets/core 
		    ${CMAKE_SOURCE_DIR}/apps/gadgetron
		    ${CMAKE_SOURCE_DIR}/apps/mriclient 
		    ${CMAKE_SOURCE_DIR}/toolboxes/ndarray
		    ${CMAKE_SOURCE_DIR}/toolboxes/gadgettools)

add_executable(siemens_mriclient main.cpp siemensraw.cpp XNode.cpp XNodeParser.cpp)

target_link_libraries(siemens_mriclient ${ACE_LIBRARIES}
  ${TINYXML_LIBRARIES} gadgettools)

install(TARGETS siemens_mriclient DESTINATION bin)
