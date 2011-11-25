cmake_policy(SET CMP0008 OLD)

add_executable(siemens_mriclient main.cpp siemensraw.cpp XNode.cpp XNodeParser.cpp)

target_link_libraries(siemens_mriclient 
				gadgettools${CMAKE_DEBUG_POSTFIX} 
				tinyxml${CMAKE_DEBUG_POSTFIX} 
				optimized ${ACE_LIBRARIES} debug ${ACE_DEBUG_LIBRARY})

install(TARGETS siemens_mriclient DESTINATION bin)
