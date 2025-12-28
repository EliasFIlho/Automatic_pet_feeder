target_include_directories(app PRIVATE .)

FILE(GLOB wifi_source ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

target_sources(app PRIVATE ${wifi_source})