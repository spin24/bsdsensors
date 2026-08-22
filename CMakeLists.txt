cmake_minimum_required (VERSION 3.16)

# metadata

project(bsdsensors)

set (bsdsensors_VERSION_MAJOR 0)
set (bsdsensors_VERSION_MINOR 1)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(BSDSENSORS_STATIC_BIN "Statically link everything into the bsdsensors executable" OFF)

if (BSDSENSORS_STATIC_BIN)
    # Make FindZLIB pick libz.a so no full-path .so sneaks past -static
    set(ZLIB_USE_STATIC_LIBS ON)
endif()

find_package(protobuf CONFIG REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)

if (BUILD_TESTING)
find_package(GTest REQUIRED)
endif()

# config

configure_file (
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
    )

include_directories("${PROJECT_BINARY_DIR}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

# subdirs

add_subdirectory(lib)
add_subdirectory(include)
add_subdirectory(proto)
if (BUILD_TESTING)
add_subdirectory(tests)
endif()

include_directories ("${PROJECT_SOURCE_DIR}/include")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# main executable

add_executable(${PROJECT_NAME}-bin src/main.cc)
set_target_properties(${PROJECT_NAME}-bin
    PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
if (BSDSENSORS_STATIC_BIN)
    find_library(PROTOBUF_STATIC_LIB NAMES libprotobuf.a HINTS /usr/local/lib)
    find_library(GLOG_STATIC_LIB NAMES libglog.a HINTS /usr/local/lib)
    find_library(GFLAGS_STATIC_LIB NAMES libgflags.a HINTS /usr/local/lib)
    if (NOT PROTOBUF_STATIC_LIB OR NOT GLOG_STATIC_LIB OR NOT GFLAGS_STATIC_LIB)
        message(FATAL_ERROR "Static libraries for protobuf/glog/gflags not found")
    endif()
    set_property(TARGET ${PROJECT_NAME}-bin PROPERTY LINK_SEARCH_START_STATIC ON)
    target_link_options(${PROJECT_NAME}-bin PRIVATE -static)
    target_link_libraries(${PROJECT_NAME}-bin
        "-Wl,--whole-archive"
        ${PROJECT_NAME}
        "-Wl,--no-whole-archive"
        ${PROTOBUF_STATIC_LIB}
        ${GLOG_STATIC_LIB}
        ${GFLAGS_STATIC_LIB}
        pthread z m
        )
else()
    target_link_libraries(${PROJECT_NAME}-bin
        ${PROJECT_NAME}
        ${PROJECT_NAME}-protos
        protobuf::libprotobuf
        gflags
        glog::glog
        )
endif()

# install

install(TARGETS ${PROJECT_NAME}-bin DESTINATION bin)
install(FILES "${PROJECT_BINARY_DIR}/config.h"
    DESTINATION include/${PROJECT_NAME})

# package

include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set (CPACK_PACKAGE_VERSION_MAJOR "${bsdsensors_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${bsdsensors_VERSION_MINOR}")
include (CPack)

include (CTest)
