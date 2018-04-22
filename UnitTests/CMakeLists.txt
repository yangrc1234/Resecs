cmake_minimum_required(VERSION 3.2)
PROJECT(resecsTests)
file(GLOB_RECURSE SOURCES ./*.cpp ./*.hpp)

include_directories(${Resecs_SOURCE_DIR}/..)
source_group("Source Files" FILES ${HEADERS} ${SOURCES})

add_subdirectory("deps/googletest")
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} resecs)
target_link_libraries(${PROJECT_NAME} gtest gtest_main)