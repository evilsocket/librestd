cmake_minimum_required(VERSION 3.0)

project(restd_library VERSION 0.1.0)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(library_INCLUDES
  include/crash_manager.h
  include/http.h
  include/http_route.h
  include/http_server.h
  include/log.h
  include/strings.h
  include/tcp_server.h
  include/tcp_stream.h
  include/consumer.hpp
  include/json.hpp
  include/restd.h
  include/work_queue.hpp)

set(library_SOURCES
  src/crash_manager.cpp
  src/http.cpp
  src/http_route.cpp
  src/http_server.cpp
  src/log.cpp
  src/strings.cpp
  src/tcp_server.cpp
  src/tcp_stream.cpp)

add_library(restd ${library_SOURCES})

target_include_directories(restd
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_options(restd
  PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Werror -pedantic>
    $<$<CONFIG:DEBUG>:-O1>
    $<$<CONFIG:RELEASE>:-O3>)

target_compile_features(restd PUBLIC
  cxx_auto_type)

set(binary_SOURCES
  hello_world.cpp)

add_executable(hello_world ${binary_SOURCES})

target_link_libraries(hello_world
    PRIVATE restd Threads::Threads)

export(TARGETS restd FILE cmake/restdConfig.cmake)
export(PACKAGE restd)

include(GNUInstallDirs)
install(TARGETS restd EXPORT restdTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(
    FILES ${library_INCLUDES}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/restd
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(cmake/restdConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)
