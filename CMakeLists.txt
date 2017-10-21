cmake_minimum_required(VERSION 3.0)

project(restd_library)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Werror -Wno-unused-value -Wno-deprecated-declarations -std=c++0x")

file( GLOB library_SOURCES src/*.cpp)

add_library(restd STATIC ${library_SOURCES})

target_include_directories(restd
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

set(binary_SOURCES
    hello_world.cpp
)

add_executable(hello_world ${binary_SOURCES})

target_link_libraries( hello_world
    PRIVATE restd pthread
)
