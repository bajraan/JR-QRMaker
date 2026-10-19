cmake_minimum_required(VERSION 3.21)

include(FetchContent)

if(NOT TARGET SFML::Graphics)
    find_package(SFML 3 COMPONENTS Graphics QUIET)
endif()

if(NOT TARGET SFML::Graphics)
    message(STATUS "SFML 3 not found on the system; fetching SFML 3.0.1")
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM
    )
    FetchContent_MakeAvailable(SFML)
endif()

add_library(jr_gui STATIC
    src/GuiApp.cpp
)

add_library(jr::gui ALIAS jr_gui)

target_include_directories(jr_gui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(jr_gui
    PUBLIC
        jr::qrcodegen
        jr::svggen
        SFML::Graphics
)

target_compile_features(jr_gui PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(jr_gui PRIVATE /W4 /permissive-)
else()
    target_compile_options(jr_gui PRIVATE -Wall -Wextra -Wpedantic)
endif()
