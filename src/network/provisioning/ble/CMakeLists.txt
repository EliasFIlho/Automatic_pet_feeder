target_include_directories(app PRIVATE .)

FILE(GLOB BLE_source ${CMAKE_CURRENT_SOURCE_DIR}/BluetoothLE.cpp)

target_sources(app PRIVATE ${BLE_source})