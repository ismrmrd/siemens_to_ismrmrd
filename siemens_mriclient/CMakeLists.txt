
add_executable(siemens_mriclient main.cpp siemensraw.cpp XNode.cpp XNodeParser.cpp)

target_link_libraries(siemens_mriclient ${ACE_LIBRARIES} ${TINYXML_LIBRARIES} gadgettools)

install(TARGETS siemens_mriclient DESTINATION bin)
