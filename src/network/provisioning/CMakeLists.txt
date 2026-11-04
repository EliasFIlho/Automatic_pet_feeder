target_include_directories(app PRIVATE .)

FILE(GLOB provisioning_source ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

target_sources(app PRIVATE ${provisioning_source})