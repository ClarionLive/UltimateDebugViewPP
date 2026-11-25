cmake_minimum_required(VERSION 3.10)
project(Scintilla)

# Scintilla library for Windows
set(SCINTILLA_SOURCES
    src/AutoComplete.cxx
    src/CallTip.cxx
    src/CaseConvert.cxx
    src/CaseFolder.cxx
    src/CellBuffer.cxx
    src/ChangeHistory.cxx
    src/CharClassify.cxx
    src/CharacterCategoryMap.cxx
    src/CharacterType.cxx
    src/ContractionState.cxx
    src/DBCS.cxx
    src/Decoration.cxx
    src/Document.cxx
    src/EditModel.cxx
    src/EditView.cxx
    src/Editor.cxx
    src/Geometry.cxx
    src/Indicator.cxx
    src/KeyMap.cxx
    src/LineMarker.cxx
    src/MarginView.cxx
    src/PerLine.cxx
    src/PositionCache.cxx
    src/RESearch.cxx
    src/RunStyles.cxx
    src/ScintillaBase.cxx
    src/Selection.cxx
    src/Style.cxx
    src/UndoHistory.cxx
    src/UniConversion.cxx
    src/UniqueString.cxx
    src/ViewStyle.cxx
    src/XPM.cxx
    
    # Windows specific
    win32/PlatWin.cxx
    win32/ScintillaDLL.cxx
    win32/ScintillaWin.cxx
    win32/HanjaDic.cxx
    
    # Call wrapper
    call/ScintillaCall.cxx
)

add_library(Scintilla STATIC ${SCINTILLA_SOURCES})

target_include_directories(Scintilla PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/win32
)

# Add Lexilla include paths for lexer interface
target_include_directories(Scintilla PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../lexilla/include
)

target_compile_definitions(Scintilla PRIVATE 
    STATIC_BUILD
    SCI_LEXER
    _CRT_SECURE_NO_WARNINGS
)

# Use static runtime
set_property(TARGET Scintilla PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")