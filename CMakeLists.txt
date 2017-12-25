cmake_minimum_required(VERSION 3.2)
PROJECT(Resecs)
file(GLOB_RECURSE HPPS ./*.hpp)
file(GLOB_RECURSE HEADERS ./*.h)

source_group("Source Files" FILES ${HEADERS} ${HPPS})
include_directories("/")

#make a dummy project.
add_executable(${PROJECT_NAME} ${HEADERS} ${HPPS})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
