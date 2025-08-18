cmake_minimum_required(VERSION 3.10)
project(fsqlf_dll)

# Source files
set(SOURCES
    ../fsqlf_temp/dll_wrapper.c
    ../fsqlf_temp/lex.yy.c
    ../fsqlf_temp/lib_fsqlf/formatter/lex_wrapper.c
    ../fsqlf_temp/lib_fsqlf/formatter/print_keywords.c
    ../fsqlf_temp/lib_fsqlf/formatter/tokque.c
    ../fsqlf_temp/lib_fsqlf/conf_file/conf_file_create.c
    ../fsqlf_temp/lib_fsqlf/conf_file/conf_file_read.c
    ../fsqlf_temp/lib_fsqlf/kw/kw.c
    ../fsqlf_temp/lib_fsqlf/kw/kwmap.c
    ../fsqlf_temp/lib_fsqlf/kw/is_word.c
    ../fsqlf_temp/lib_fsqlf/lex/token.c
    ../fsqlf_temp/utils/queue/queue.c
    ../fsqlf_temp/utils/stack/stack.c
    ../fsqlf_temp/utils/string/read_int.c
)

# Create DLL
add_library(fsqlf SHARED ${SOURCES})

# Include directories
target_include_directories(fsqlf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../fsqlf_temp
    ${CMAKE_CURRENT_SOURCE_DIR}/../fsqlf_temp/lib_fsqlf
    ${CMAKE_CURRENT_SOURCE_DIR}/../fsqlf_temp/lib_fsqlf/formatter
    ${CMAKE_CURRENT_SOURCE_DIR}/../fsqlf_temp/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/../fsqlf_temp/include
)

# Compiler definitions
target_compile_definitions(fsqlf PRIVATE
    BUILDING_LIBFSQLF
    _CRT_SECURE_NO_WARNINGS
)

# Set C standard
set_target_properties(fsqlf PROPERTIES C_STANDARD 99)