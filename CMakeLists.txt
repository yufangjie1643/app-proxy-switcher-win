cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(CodexProxySwitcher VERSION 2.3.1 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(SOURCES
    src/main.cpp
    src/utils.cpp
    src/dpi_utils.cpp
    src/known_agents.cpp
    src/config_store.cpp
    src/app_finder.cpp
    src/proxy_detector.cpp
    src/launcher.cpp
    src/system_proxy.cpp
    src/app_window.cpp
    src/proxy_dialog.cpp
    src/app_select_dialog.cpp
    src/resource.rc
)

set(COMMON_SOURCES
    src/utils.cpp
    src/dpi_utils.cpp
    src/known_agents.cpp
    src/config_store.cpp
    src/app_finder.cpp
    src/proxy_detector.cpp
    src/launcher.cpp
    src/system_proxy.cpp
    src/proxy_dialog.cpp
    src/app_select_dialog.cpp
    src/resource.rc
)

set(HEADERS
    src/utils.hpp
    src/dpi_utils.hpp
    src/known_agents.hpp
    src/app_config.hpp
    src/proxy_mode.hpp
    src/config_store.hpp
    src/app_finder.hpp
    src/proxy_detector.hpp
    src/launcher.hpp
    src/system_proxy.hpp
    src/app_window.hpp
    src/proxy_dialog.hpp
    src/app_select_dialog.hpp
    src/resource.h
)

add_executable(CodexProxySwitcher WIN32 ${SOURCES} ${HEADERS})

if(MSVC)
    target_link_options(CodexProxySwitcher PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_options(CodexProxySwitcher PRIVATE -mwindows)
endif()

# Embed manifest for DPI awareness and UTF-8 code page
if(MSVC)
    target_sources(CodexProxySwitcher PRIVATE src/app.manifest)
endif()

target_compile_definitions(CodexProxySwitcher PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

if(MSVC)
    target_compile_options(CodexProxySwitcher PRIVATE /utf-8)
endif()

target_link_libraries(CodexProxySwitcher PRIVATE
    shell32
    ole32
    shlwapi
    comctl32
    user32
    gdi32
    advapi32
    ws2_32
)

set_target_properties(CodexProxySwitcher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/build/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/build/bin
)

option(BUILD_WEBVIEW2 "Build the WebView2 UI executable" ON)
set(WEBVIEW2_SDK_DIR "" CACHE PATH "Path to extracted Microsoft.Web.WebView2 NuGet package")
set(WEBVIEW2_SDK_VERSION "1.0.3967.48" CACHE STRING "Microsoft.Web.WebView2 SDK version")

if(BUILD_WEBVIEW2)
    if(NOT WEBVIEW2_SDK_DIR)
        set(WEBVIEW2_PINNED_HEADER "${CMAKE_SOURCE_DIR}/build/_deps/webview2/${WEBVIEW2_SDK_VERSION}/build/native/include/WebView2.h")
        if(EXISTS "${WEBVIEW2_PINNED_HEADER}")
            set(WEBVIEW2_HEADER "${WEBVIEW2_PINNED_HEADER}")
            get_filename_component(WEBVIEW2_INCLUDE_DIR "${WEBVIEW2_HEADER}" DIRECTORY)
            get_filename_component(WEBVIEW2_NATIVE_DIR "${WEBVIEW2_INCLUDE_DIR}" DIRECTORY)
            get_filename_component(WEBVIEW2_BUILD_DIR "${WEBVIEW2_NATIVE_DIR}" DIRECTORY)
            get_filename_component(WEBVIEW2_SDK_DIR "${WEBVIEW2_BUILD_DIR}" DIRECTORY)
        else()
            file(GLOB WEBVIEW2_HEADERS "${CMAKE_SOURCE_DIR}/build/_deps/webview2/*/build/native/include/WebView2.h")
            list(SORT WEBVIEW2_HEADERS)
            if(WEBVIEW2_HEADERS)
                list(GET WEBVIEW2_HEADERS -1 WEBVIEW2_HEADER)
                get_filename_component(WEBVIEW2_INCLUDE_DIR "${WEBVIEW2_HEADER}" DIRECTORY)
                get_filename_component(WEBVIEW2_NATIVE_DIR "${WEBVIEW2_INCLUDE_DIR}" DIRECTORY)
                get_filename_component(WEBVIEW2_BUILD_DIR "${WEBVIEW2_NATIVE_DIR}" DIRECTORY)
                get_filename_component(WEBVIEW2_SDK_DIR "${WEBVIEW2_BUILD_DIR}" DIRECTORY)
            endif()
        endif()
    endif()

    if(NOT WEBVIEW2_SDK_DIR OR NOT EXISTS "${WEBVIEW2_SDK_DIR}/build/native/include/WebView2.h")
        message(FATAL_ERROR "WebView2 SDK not found. Run scripts/Fetch-WebView2.ps1 before configuring, or set WEBVIEW2_SDK_DIR.")
    endif()

    set(WEBVIEW2_ARCH "x64")
    set(WEBVIEW2_INCLUDE_DIR "${WEBVIEW2_SDK_DIR}/build/native/include")
    set(WEBVIEW2_LOADER_STATIC "${WEBVIEW2_SDK_DIR}/build/native/${WEBVIEW2_ARCH}/WebView2LoaderStatic.lib")
    if(NOT EXISTS "${WEBVIEW2_LOADER_STATIC}")
        message(FATAL_ERROR "WebView2 static loader not found: ${WEBVIEW2_LOADER_STATIC}")
    endif()

    add_executable(CodexProxySwitcherWebView2 WIN32
        src/webview_main.cpp
        src/webview_window.cpp
        src/webview_window.hpp
        src/webview_ui.hpp
        ${COMMON_SOURCES}
        ${HEADERS}
        src/app.manifest
    )

    target_include_directories(CodexProxySwitcherWebView2 PRIVATE "${WEBVIEW2_INCLUDE_DIR}")

    target_compile_definitions(CodexProxySwitcherWebView2 PRIVATE
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )

    if(MSVC)
        target_compile_options(CodexProxySwitcherWebView2 PRIVATE /utf-8)
        target_link_options(CodexProxySwitcherWebView2 PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup)
    endif()

    target_link_libraries(CodexProxySwitcherWebView2 PRIVATE
        "${WEBVIEW2_LOADER_STATIC}"
        shell32
        ole32
        shlwapi
        comctl32
        user32
        gdi32
        advapi32
        ws2_32
        version
    )

    set_target_properties(CodexProxySwitcherWebView2 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/build/bin
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/build/bin
    )
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(app_finder_tests
        tests/app_finder_tests.cpp
        src/dpi_utils.cpp
        src/known_agents.cpp
        src/app_finder.cpp
        src/proxy_detector.cpp
        src/utils.cpp
    )

    target_compile_definitions(app_finder_tests PRIVATE
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )

    if(MSVC)
        target_compile_options(app_finder_tests PRIVATE /utf-8)
    endif()

    target_link_libraries(app_finder_tests PRIVATE shell32 ole32 user32 ws2_32)
    add_test(NAME app_finder_tests COMMAND app_finder_tests)
endif()
