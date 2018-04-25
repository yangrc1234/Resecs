cmake_minimum_required(VERSION 3.2)
PROJECT(resecs)
file(GLOB_RECURSE HPPS ./*.hpp)
file(GLOB_RECURSE HEADERS ./*.h)
file(GLOB_RECURSE SOURCES ./*.cpp)

source_group("Source Files" FILES ${SOURCES} ${HEADERS} ${HPPS})
include_directories("/")

add_library(${PROJECT_NAME} ${HEADERS} ${HPPS} ${SOURCES} )

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)