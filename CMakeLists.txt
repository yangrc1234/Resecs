cmake_minimum_required(VERSION 3.2)

add_subdirectory("./Resecs")

option(buildTest "Should test be built" OFF)
if (buildTest)
	add_subdirectory("./ResecsTests")
endif()