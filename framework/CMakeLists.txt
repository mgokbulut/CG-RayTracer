cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(ComputerGraphics CXX)

set(PACKAGE_MANAGER "FetchContent" CACHE STRING "The package manager used to download the third-party dependencies")

# Glad requires Python to configure itself and Python is an annoying dependency to have.
# Also, glad doesn't work well on Windows with Conan. It will insert error checks in all
# OpenGL2 calls which then create errors because error checks are not supported in legacy
# OpenGL... . So as an exception to the rule we include the glad src/header files directly.
include("cmake/download_framework_packages.cmake")
include("cmake/download_optional_packages.cmake")

# Find the OpenGL/GLUT header and library files (as CMake targets).
set(OpenGL_GL_PREFERENCE GLVND) # Prevent CMake warning about legacy fallback on Linux.
find_package(OpenGL REQUIRED)

set(PLATFORM_SPECIFIC_TARGETS "")
if (UNIX)
	# On some older Linux compilers (e.g. gcc8) the standard template library filesystem (C++17) requires explicitely linking to it.
	# The Findfilesystem.cmake file located in the cmake folder will take care of creating a filesystem target. It was copied from:
	# https://github.com/inviwo/inviwo/blob/master/cmake/modules/FindFilesystem.cmake
	set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")
	find_package(Filesystem REQUIRED)
	set(PLATFORM_SPECIFIC_TARGETS std::filesystem ${CMAKE_DL_LIBS})
endif()

# Slightly modified versions of the files from the cpp-starter-project by Jason Turner:
# https://github.com/lefticus/cpp_starter_project/
include("cmake/CompilerWarnings.cmake") # Enable almost all compiler warnings and CMake option to enable -Werror.
include("cmake/Sanitizers.cmake") # CMake options to enable address, memory, UB and thread sanitizers.
include("cmake/StaticAnalyzers.cmake") # CMake options to enable clang-tidy or cpp-check.

add_library(CGFrameworkBase INTERFACE)
target_link_libraries(CGFrameworkBase INTERFACE ${FRAMEWORK_NON_GRAPHICS_TARGETS})

add_library(CGFramework STATIC
	"src/trackball.cpp"
	"src/window.cpp"
	"src/shader.cpp"
	"src/imgui_impl_glfw.cpp"
	"src/imgui_impl_opengl2.cpp"
	"src/imgui_impl_opengl3.cpp"
)
target_include_directories(CGFramework PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include/")
target_link_libraries(CGFramework PUBLIC OpenGL::GL OpenGL::GLU ${FRAMEWORK_GRAPHICS_TARGETS} CGFrameworkBase ${PLATFORM_SPECIFIC_TARGETS})
target_compile_features(CGFramework PUBLIC cxx_std_17) # C++17
# Prevent accidentaly picking up a system-wide or vcpkg install of another loader (e.g. GLEW).
target_compile_definitions(CGFramework PUBLIC "-DIMGUI_IMPL_OPENGL_LOADER_GLAD=1")
