target_include_directories(app PRIVATE .)

FILE(GLOB common_source ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

target_sources(app PRIVATE ${common_source})