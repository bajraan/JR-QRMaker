cmake_minimum_required(VERSION 3.21)

# Project metadata
project(JR-QRMaker
    VERSION 1.0.0
    DESCRIPTION "AI-in-the-Loop QR Code & SVG generator built with modern C++ and CMake"
    LANGUAGES CXX
)

include(GNUInstallDirs)

# Keep non-code application assets available beside the build tree and in
# installed packages. CONFIGURE_DEPENDS refreshes this list when assets are
# added or removed, while the always-built target copies their current contents.
file(GLOB_RECURSE JR_QRMAKER_RESOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/*"
)
add_custom_target(jr_qrmaker_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        "${CMAKE_CURRENT_BINARY_DIR}/resources"
    DEPENDS ${JR_QRMAKER_RESOURCE_FILES}
    COMMENT "Copying JR-QRMaker application resources"
    VERBATIM
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/jr-qrmaker/resources"
)

# Enforce C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for language servers (clangd, VS Code, C++ tools)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add internal modular subdirectories
add_subdirectory(modules/qrcodegen)
add_subdirectory(modules/svggen)
add_subdirectory(modules/qrstyle)

option(JR_QRMAKER_BUILD_GUI "Build the SFML GUI application" ON)
if(JR_QRMAKER_BUILD_GUI)
    add_subdirectory(modules/gui)
endif()

# Main application executable target (CLI)
add_executable(JR-QRMaker src/main.cpp)
add_dependencies(JR-QRMaker jr_qrmaker_resources)

# Link core module libraries using modern CMake alias targets
target_link_libraries(JR-QRMaker
    PRIVATE
        jr::qrcodegen
        jr::svggen
)

# Set compile options for main executable based on compiler family
if(MSVC)
    target_compile_options(JR-QRMaker PRIVATE /W4 /permissive-)
else()
    target_compile_options(JR-QRMaker PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(JR_QRMAKER_BUILD_GUI)
    add_executable(JR-QRMaker-gui src/gui_main.cpp)
    add_dependencies(JR-QRMaker-gui jr_qrmaker_resources)
    target_link_libraries(JR-QRMaker-gui PRIVATE jr::gui)

    if(MSVC)
        target_compile_options(JR-QRMaker-gui PRIVATE /W4 /permissive-)
    else()
        target_compile_options(JR-QRMaker-gui PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

