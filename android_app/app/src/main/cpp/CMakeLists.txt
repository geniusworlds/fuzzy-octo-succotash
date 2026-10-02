cmake_minimum_required(VERSION 3.10.2)
project("ios_vm_engine")

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../ios_engine)

add_library(ios_vm SHARED
    native_bridge.cpp
    ../../../../ios_engine/macho_loader.cpp
    ../../../../ios_engine/darwin_runtime.cpp
    ../../../../ios_engine/arm64_core.cpp
    ../../../../ios_engine/ios_compositor.cpp
)

find_library(log-lib log)
find_library(jnigraphics-lib jnigraphics)

target_link_libraries(ios_vm
    ${log-lib}
    ${jnigraphics-lib}
)
