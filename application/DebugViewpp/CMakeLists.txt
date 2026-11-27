project(UltimateDebugviewpp)

add_executable(UltimateDebugviewpp
    AboutDlg.cpp
    ClarionProcedureView.cpp
    ClarionProcedureTimelineView.cpp
    CLogViewTabItem2.cpp
    CQueueGridView.cpp
    FloatingTabWindow.cpp
    FloatingMainTabWindow.cpp
    DarkMode.cpp
    DebugView++.cpp
    DropTargetSupport.cpp
    FileOptionDlg.cpp
    FilterDlg.cpp
    FilterPage.cpp
    FindDlg.cpp
    Grid.cpp
    HelpTabView.cpp
    HistoryDlg.cpp
    LogView.cpp
    MainFrame.cpp
    McpAgentSetup.cpp
    McpServer.cpp
    McpTools.cpp
    ModernToolbar.cpp
    PreferencesDlg.cpp
    RegExDlg.cpp
    RenameProcessDlg.cpp
    ManageLayoutsDlg.cpp
    RunDlg.cpp
    ScintillaWrapper.cpp
    SourceDlg.cpp
    SourcesDlg.cpp
    DebugView++.rc
)

target_link_libraries(UltimateDebugviewpp
    PRIVATE
        project::definitions
        project::compile_features
        project::compile_options
        project::gui_link_options
        nuget::boost
        nuget::wtl
        nuget::webview2
        dv::cobaltfusion
        dv::tabbingframework
        dv::propertygrid
        dv::indexedstorage
        dv::library
        dv::win32
        Scintilla
        Lexilla
        imm32.lib
        shlwapi.lib
        ole32.lib
        oleaut32.lib
        ws2_32.lib
        mswsock.lib
        bcrypt.lib
)

# Boost.JSON is compiled header-only into McpServer.cpp; Asio/Beast are header-only.
target_compile_definitions(UltimateDebugviewpp PRIVATE BOOST_JSON_NO_LIB BOOST_CONTAINER_NO_LIB BOOST_ASIO_NO_DEPRECATED)
target_compile_options(UltimateDebugviewpp PRIVATE /bigobj)

target_include_directories(UltimateDebugviewpp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/scintilla/include>
)
# In-app help lives in <repo>/help and is copied next to the executable.
add_custom_command(TARGET UltimateDebugviewpp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/help" "$<TARGET_FILE_DIR:UltimateDebugviewpp>/help")
