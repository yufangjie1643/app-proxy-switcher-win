cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(CodexProxySwitcher VERSION 2.1.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(SOURCES
    src/main.cpp
    src/utils.cpp
    src/known_agents.cpp
    src/config_store.cpp
    src/app_finder.cpp
    src/launcher.cpp
    src/system_proxy.cpp
    src/app_window.cpp
    src/proxy_dialog.cpp
    src/app_select_dialog.cpp
    src/resource.rc
)

set(HEADERS
    src/utils.hpp
    src/known_agents.hpp
    src/app_config.hpp
    src/proxy_mode.hpp
    src/config_store.hpp
    src/app_finder.hpp
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
)

set_target_properties(CodexProxySwitcher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/build/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/build/bin
)

include(CTest)
if(BUILD_TESTING)
    add_executable(app_finder_tests
        tests/app_finder_tests.cpp
        src/known_agents.cpp
        src/app_finder.cpp
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

    target_link_libraries(app_finder_tests PRIVATE shell32 ole32)
    add_test(NAME app_finder_tests COMMAND app_finder_tests)
endif()
