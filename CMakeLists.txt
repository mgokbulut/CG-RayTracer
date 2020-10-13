cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(ComputerGraphics CXX)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/framework")
	# Create framework library and include CMake scripts (compiler warnings, sanitizers and static analyzers).
	add_subdirectory("framework")
else()
	# During development the framework lives in parent folder.
	add_subdirectory("../../../framework/" "${CMAKE_BINARY_DIR}/framework/")
endif()
get_optional_packages(TARGET OptionalPackages PACKAGES "catch2" "assimp" "stb")

add_executable(FinalProject2
	"src/main.cpp"
	"src/ray_tracing.cpp"
	"src/scene.cpp"
	"src/mesh.cpp"
	"src/draw.cpp"
	"src/screen.cpp"
	"src/bounding_volume_hierarchy.cpp"
	"src/image.cpp"
	"src/stb_image.cpp")
# Link to all dependencies / make their header files available.
target_link_libraries(FinalProject2 PRIVATE CGFramework OptionalPackages)
target_compile_features(FinalProject2 PRIVATE cxx_std_17) # C++17
enable_sanitizers(FinalProject2)
set_project_warnings(FinalProject2)

find_package(OpenMP)
if (OpenMP_FOUND)
	target_link_libraries(FinalProject2 PRIVATE OpenMP::OpenMP_CXX)
	target_compile_definitions(FinalProject2 PRIVATE "-DUSE_OPENMP=1")
endif()

target_compile_definitions(FinalProject2 PRIVATE
	"-DDATA_DIR=\"${CMAKE_CURRENT_LIST_DIR}/data/\""
	"-DOUTPUT_DIR=\"${CMAKE_CURRENT_LIST_DIR}/\"")
