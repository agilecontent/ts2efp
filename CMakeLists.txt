cmake_minimum_required(VERSION 3.15)
project(ts2efp)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

include(ExternalProject)
ExternalProject_Add(project_mpegts
        GIT_REPOSITORY https://github.com/andersc/mpegts
        GIT_SUBMODULES ""
        UPDATE_COMMAND git pull
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mpegts
        BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mpegts
        GIT_PROGRESS 1
        CONFIGURE_COMMAND cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/mpegts
        BUILD_COMMAND cmake --build ${CMAKE_CURRENT_SOURCE_DIR}/mpegts --config ${CMAKE_BUILD_TYPE} --target mpegts
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        INSTALL_COMMAND ""
        )
add_library(mpegts STATIC IMPORTED)
set_property(TARGET mpegts PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/mpegts/libmpegts.a)

ExternalProject_Add(project_efp
        GIT_REPOSITORY https://github.com/Unit-X/efp.git
        GIT_SUBMODULES ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/efp
        BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/efp
        GIT_PROGRESS 1
        CONFIGURE_COMMAND cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${CMAKE_CURRENT_SOURCE_DIR}/efp
        BUILD_COMMAND cmake --build ${CMAKE_CURRENT_SOURCE_DIR}/efp --config ${CMAKE_BUILD_TYPE} --target efp
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        INSTALL_COMMAND ""
        )
add_library(efp STATIC IMPORTED)
set_property(TARGET efp PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/efp/libefp.a)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mpegts/amf0/amf0/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mpegts/mpegts/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/efp/)

add_executable(ts2efp ts2efp.cpp)
add_dependencies(ts2efp project_mpegts)
add_dependencies(ts2efp project_efp)
target_link_libraries(ts2efp efp mpegts)

add_executable(efp2ts efp2ts.cpp)
add_dependencies(efp2ts project_mpegts)
add_dependencies(efp2ts project_efp)
target_link_libraries(efp2ts efp mpegts)