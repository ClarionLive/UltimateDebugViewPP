cmake_minimum_required(VERSION 3.10)
project(Lexilla)

# Collect all lexer source files
file(GLOB LEXER_SOURCES 
    src/Lexilla.cxx
    lexers/*.cxx
    lexlib/*.cxx
)

add_library(Lexilla STATIC ${LEXER_SOURCES})

target_include_directories(Lexilla PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lexlib
    ${CMAKE_SOURCE_DIR}/scintilla/include
)

target_compile_definitions(Lexilla PRIVATE 
    STATIC_BUILD
    _CRT_SECURE_NO_WARNINGS
)

# Use static runtime
set_property(TARGET Lexilla PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")