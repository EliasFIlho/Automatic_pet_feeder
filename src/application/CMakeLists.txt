target_include_directories(app PRIVATE .)

FILE(GLOB sources *.cpp)

target_sources(app PRIVATE ${sources})