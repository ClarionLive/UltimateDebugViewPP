project(UltimateDebugviewpp)

add_executable(UltimateDebugviewpp
    AboutDlg.cpp
    CLogViewTabItem2.cpp
    DebugView++.cpp
    DropTargetSupport.cpp
    FileOptionDlg.cpp
    FilterDlg.cpp
    FilterPage.cpp
    FindDlg.cpp
    Grid.cpp
    HistoryDlg.cpp
    LogView.cpp
    MainFrame.cpp
    ModernToolbar.cpp
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
        dv::cobaltfusion
        dv::tabbingframework
        dv::propertygrid
        dv::indexedstorage
        dv::library
        dv::win32
        Scintilla
        Lexilla
        imm32.lib
)

install(TARGETS UltimateDebugviewpp DESTINATION bin)

target_include_directories(UltimateDebugviewpp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/scintilla/include>
)

install(CODE "
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/UltimateDebugviewpp.exe ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/UltimateDebugviewpp-small.exe)
  execute_process(COMMAND ${PROJECT_UTILS_DIRECTORY}/upx.exe ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/UltimateDebugviewpp-small.exe)
")

INSTALL(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/UltimateDebugviewpp-small.exe DESTINATION bin)
