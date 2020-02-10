cmake_minimum_required(VERSION 3.15)
project(ts2efp)

set(CMAKE_CXX_STANDARD 17)

include(ExternalProject)
ExternalProject_Add(project_mpegts
        GIT_REPOSITORY https://github.com/andersc/mpegts
        GIT_SUBMODULES ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mpegts
        BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mpegts
        GIT_PROGRESS 1
        BUILD_COMMAND make mpegts
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        INSTALL_COMMAND ""
        )
add_library(mpegts STATIC IMPORTED)
set_property(TARGET mpegts PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/mpegts/libmpegts.a)

ExternalProject_Add(project_efp
        GIT_REPOSITORY https://bitbucket.org/unitxtra/efp.git
        GIT_SUBMODULES ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/efp
        BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/efp
        GIT_PROGRESS 1
        BUILD_COMMAND make efp
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        INSTALL_COMMAND ""
        )
add_library(efp STATIC IMPORTED)
set_property(TARGET efp PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/efp/libefp.a)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mpegts/mpegts/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/efp/)

add_executable(ts2efp main.cpp)
add_dependencies(ts2efp project_mpegts)
add_dependencies(ts2efp project_efp)
target_link_libraries(ts2efp efp mpegts)