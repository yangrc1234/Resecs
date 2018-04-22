cmake_minimum_required(VERSION 3.2)

add_subdirectory("./Resecs")

option(Resecs_BuildTest "Should test be built" OFF)
if (Resecs_BuildTest)
	add_subdirectory("./UnitTests")
endif()