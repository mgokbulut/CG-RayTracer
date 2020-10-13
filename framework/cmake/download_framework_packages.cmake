add_subdirectory("glad")

if (PACKAGE_MANAGER STREQUAL "FetchContent")
	# === Download, configure and compile all the third-party dependencies ===
	include(FetchContent)
	# We only want the library itself, no additional example/test/tool applications.
	set(GLFW_BUILD_EXAMPLES CACHE BOOL OFF)
	set(GLFW_BUILD_TESTS CACHE BOOL OFF)
	set(GLFW_BUILD_DOCS CACHE BOOL OFF)
	set(GLFW_BUILD_INSTALL CACHE BOOL OFF)
	FetchContent_Declare(glfw
		GIT_REPOSITORY "https://github.com/glfw/glfw.git"
		GIT_TAG "3.3.2" # https://github.com/glfw/glfw/tags
		GIT_SHALLOW TRUE)
	#FetchContent_Declare(glad
	#	GIT_REPOSITORY "https://github.com/Dav1dde/glad.git"
	#	GIT_TAG "v0.1.33" # https://github.com/Dav1dde/glad/tags
	#	GIT_SHALLOW TRUE)
	FetchContent_Declare(glm
		GIT_REPOSITORY "https://github.com/g-truc/glm.git"
		GIT_TAG "0.9.9.8" # https://github.com/g-truc/glm/tags
		GIT_SHALLOW TRUE)
	# NOTE(Mathijs): remove GSL dependency when we can safely assume every student can install a C++20 compatible standard library.
	FetchContent_Declare(gsllite
		GIT_REPOSITORY "https://github.com/gsl-lite/gsl-lite.git"
		GIT_TAG "v0.37.0" # https://github.com/gsl-lite/gsl-lite/tags
		GIT_SHALLOW TRUE)
	# NOTE(Mathijs): remove fmt dependency when we can safely assume every student can install a C++20 compatible standard library.
	FetchContent_Declare(fmt
		GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
		GIT_TAG "7.0.3" # https://github.com/fmtlib/fmt/tags
		GIT_SHALLOW TRUE)
	FetchContent_MakeAvailable(glfw glm gsllite fmt)

	# Imgui is not a CMake project so we need some manual labour here (FetchContent Declare+MakeAvailable only works with CMake projects).
	FetchContent_Declare(imgui
	  GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
	  GIT_TAG "v1.77" # https://github.com/ocornut/imgui/tags
	  GIT_SHALLOW TRUE)
	if(NOT imgui_POPULATED)
	  FetchContent_Populate(imgui)
	endif()
	add_library(imgui
		"${imgui_SOURCE_DIR}/imgui.cpp"
		"${imgui_SOURCE_DIR}/imgui_demo.cpp"
		"${imgui_SOURCE_DIR}/imgui_draw.cpp"
		"${imgui_SOURCE_DIR}/imgui_widgets.cpp"
		"${imgui_SOURCE_DIR}/imgui.h"
		"${imgui_SOURCE_DIR}/imgui_internal.h"
		)
	target_include_directories(imgui PUBLIC "${imgui_SOURCE_DIR}")
	target_link_libraries(imgui PUBLIC glfw glad)

	target_compile_options(gsl-lite-v1 INTERFACE "-DGSL_THROW_ON_CONTRACT_VIOLATION=1")

	set(FRAMEWORK_GRAPHICS_TARGETS glfw glad imgui glm)
	set(FRAMEWORK_NON_GRAPHICS_TARGETS gsl-lite-v1 fmt::fmt)
elseif ((PACKAGE_MANAGER STREQUAL "vcpkg") OR (PACKAGE_MANAGER STREQUAL "system"))
	find_package(glfw3 CONFIG REQUIRED)
	#find_package(glad CONFIG REQUIRED)
	find_package(imgui CONFIG REQUIRED)
	find_package(glm CONFIG REQUIRED)
	find_package(gsl-lite CONFIG REQUIRED) # Remove in C++20
	find_package(fmt CONFIG REQUIRED) # Remove in C++20

	target_compile_options(gsl::gsl-lite-v1 INTERFACE "-DGSL_THROW_ON_CONTRACT_VIOLATION=1")

	set(FRAMEWORK_GRAPHICS_TARGETS glfw glad imgui::imgui glm)
	set(FRAMEWORK_NON_GRAPHICS_TARGETS gsl::gsl-lite-v1 fmt::fmt)
elseif (PACKAGE_MANAGER STREQUAL "conan")
	include("cmake/conan.cmake")
	conan_cmake_run(
		REQUIRES "glfw/3.3.2" "glm/0.9.9.8" "gsl-lite/0.37.0" "imgui/1.77" "fmt/7.0.3"
		OPTIONS "gsl-lite:on_contract_violation=throw"
		BASIC_SETUP CMAKE_TARGETS
		BUILD missing)

	target_compile_options(CONAN_PKG::gsl-lite INTERFACE "-Dgsl_CONFIG_DEFAULTS_VERSION=1")
	set(FRAMEWORK_GRAPHICS_TARGETS CONAN_PKG::glfw CONAN_PKG::imgui CONAN_PKG::glm glad)
	set(FRAMEWORK_NON_GRAPHICS_TARGETS CONAN_PKG::gsl-lite CONAN_PKG::fmt)
else()
	message(FATAL_ERROR "Unknown package manager ${PACKAGE_MANAGER}")
endif()
