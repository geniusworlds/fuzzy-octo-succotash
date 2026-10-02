cmake_minimum_required(VERSION 3.10.2)
project("ios_vm_engine")

set(CMAKE_CXX_STANDARD 17)

set(IOS_ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../ios_engine)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IOS_ENGINE_DIR}
)

add_library(ios_vm SHARED
    native_bridge.cpp
    ${IOS_ENGINE_DIR}/macho_loader.cpp
    ${IOS_ENGINE_DIR}/darwin_runtime.cpp
    ${IOS_ENGINE_DIR}/arm64_core.cpp
    ${IOS_ENGINE_DIR}/ios_compositor.cpp
)

find_library(log-lib log)
find_library(jnigraphics-lib jnigraphics)

target_link_libraries(ios_vm
    ${log-lib}
    ${jnigraphics-lib}
)
