# Centralized Dependencies Configuration

# Boost (from centralized location)
set(BOOST_ROOT "C:/Libraries/boost_1_80_0" CACHE PATH "Boost root directory")
message(STATUS "Using Boost from: ${BOOST_ROOT}")

add_library(centralized_boost INTERFACE)
target_include_directories(centralized_boost INTERFACE "${BOOST_ROOT}")
target_link_directories(centralized_boost INTERFACE "${BOOST_ROOT}/lib")
add_library(nuget::boost ALIAS centralized_boost)
add_library(nuget::boost_test ALIAS centralized_boost)

# WTL (from centralized location)
set(WTL_ROOT "C:/Libraries/wtl_10_0_10320" CACHE PATH "WTL root directory")
message(STATUS "Using WTL from: ${WTL_ROOT}")

add_library(centralized_wtl INTERFACE)
target_include_directories(centralized_wtl INTERFACE "${WTL_ROOT}")
add_library(nuget::wtl ALIAS centralized_wtl)

# Snappy (from centralized location)
set(SNAPPY_ROOT "C:/Libraries/snappy_1_1_1_7" CACHE PATH "Snappy root directory")
message(STATUS "Using Snappy from: ${SNAPPY_ROOT}")

add_library(centralized_snappy "${SNAPPY_ROOT}/src/snappy-single-file.cpp")
target_compile_definitions(centralized_snappy PUBLIC SNAPPY_STATIC)
target_include_directories(centralized_snappy PUBLIC "${SNAPPY_ROOT}/include")
add_library(nuget::snappy ALIAS centralized_snappy)

# WebView2 (Microsoft Edge WebView2)
# Download WebView2 SDK if not present
set(WEBVIEW2_VERSION "1.0.2210.55" CACHE STRING "WebView2 SDK version")
set(WEBVIEW2_ROOT "${CMAKE_BINARY_DIR}/packages/Microsoft.Web.WebView2.${WEBVIEW2_VERSION}" CACHE PATH "WebView2 SDK directory")

if(NOT EXISTS "${WEBVIEW2_ROOT}")
    message(STATUS "Downloading WebView2 SDK version ${WEBVIEW2_VERSION}...")
    file(DOWNLOAD
        "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${WEBVIEW2_VERSION}"
        "${CMAKE_BINARY_DIR}/packages/webview2.${WEBVIEW2_VERSION}.zip"
        SHOW_PROGRESS
    )
    file(ARCHIVE_EXTRACT
        INPUT "${CMAKE_BINARY_DIR}/packages/webview2.${WEBVIEW2_VERSION}.zip"
        DESTINATION "${CMAKE_BINARY_DIR}/packages/Microsoft.Web.WebView2.${WEBVIEW2_VERSION}"
    )
endif()

message(STATUS "Using WebView2 from: ${WEBVIEW2_ROOT}")

add_library(webview2 INTERFACE)
target_include_directories(webview2 INTERFACE "${WEBVIEW2_ROOT}/build/native/include")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_link_libraries(webview2 INTERFACE "${WEBVIEW2_ROOT}/build/native/x64/WebView2LoaderStatic.lib")
else()
    target_link_libraries(webview2 INTERFACE "${WEBVIEW2_ROOT}/build/native/x86/WebView2LoaderStatic.lib")
endif()
add_library(nuget::webview2 ALIAS webview2)