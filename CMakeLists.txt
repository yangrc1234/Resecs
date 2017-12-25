cmake_minimum_required(VERSION 3.2)
PROJECT(Resecs)
file(GLOB_RECURSE HEADERS ./*.hpp)
SET(HEADERS "${HEADERS}")

source_group("Source Files" FILES "${HEADERS}")
MESSAGE(${HEADERS})

include_directories("/")

#make a dummy project.
add_executable(${PROJECT_NAME} ${HEADERS})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
