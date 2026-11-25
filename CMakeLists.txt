cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CMakeLists.buildnumber)

MESSAGE("Building UltimateDebugViewPP version: ${DEBUGVIEW_VERSION}")

project(
  UltimateDebugViewPP
  VERSION ${DEBUGVIEW_VERSION}
  DESCRIPTION "UltimateDebugViewPP - Enhanced debug viewer with modern UI"
  HOMEPAGE_URL "https://github.com/CobaltFusion/DebugViewPP"
  LANGUAGES CXX C
)

add_compile_options(/MP)

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
endif()

# Project-wide compile definitions and options
add_library(project_definitions INTERFACE)
add_library(project_compile_features INTERFACE)
add_library(project_compile_options INTERFACE)
add_library(msvc_gui_link_options INTERFACE)

target_compile_features(project_compile_features INTERFACE cxx_constexpr cxx_auto_type cxx_final cxx_std_20)
target_compile_definitions(project_definitions
	INTERFACE
	UNICODE
	NOMINMAX
	WIN32_LEAN_AND_MEAN
	_CRT_SECURE_NO_WARNINGS
	_WINSOCK_DEPRECATED_NO_WARNINGS
	WIN32
	WIN64
	_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	"_WIN32_WINNT=0x0601"
)

target_compile_options(project_compile_options INTERFACE /utf-8 /Oy)
target_link_options(msvc_gui_link_options INTERFACE /SUBSYSTEM:WINDOWS /NOLOGO /RELEASE)

# Warnings
target_compile_options(project_compile_options INTERFACE /W4 /w14242 /w14254 /w14263 /w14265 /w14287 /w14289 /w14296 /w14311 /w14545 /w14546 /w14547 /w14549 /w14555 /w14640 /w14826 /w14905 /w14906 /w14928 /we4715)

add_library(project::definitions ALIAS project_definitions)
add_library(project::compile_features ALIAS project_compile_features)
add_library(project::compile_options ALIAS project_compile_options)
add_library(project::gui_link_options ALIAS msvc_gui_link_options)

# Include centralized dependencies configuration
include(cmake/CMakeLists.dependencies.cmake)

# Add subdirectories
add_subdirectory(scintilla)
add_subdirectory(lexilla)
add_subdirectory(fsqlf_dll)
add_subdirectory(application)