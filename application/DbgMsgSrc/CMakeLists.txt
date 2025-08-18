project(DbgMsgSrc)

add_executable(${PROJECT_NAME} DbgMsgSrc.cpp Timer.cpp)

# Add the new test message generator
add_executable(DebugViewTestMessages DebugViewTestMessages.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        project::definitions
        project::compile_features
        project::compile_options
        project::console_link_options
        nuget::boost
        dv::cobaltfusion
        dv::library
        dv::win32
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

# Configure DebugViewTestMessages executable with same settings as DbgMsgSrc
target_link_libraries(DebugViewTestMessages
    PRIVATE
        project::definitions
        project::compile_features
        project::compile_options
        project::console_link_options
)

target_include_directories(DebugViewTestMessages PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

# not part of the release?
#install(TARGETS ${PROJECT_NAME} DESTINATION bin)
#install(TARGETS DebugViewTestMessages DESTINATION bin)
