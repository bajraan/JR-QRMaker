cmake_minimum_required(VERSION 3.20)

# Define static library target for SVG generator module
add_library(jr_svggen STATIC
    src/SvgWriter.cpp
)

# Create namespaced alias target
add_library(jr::svggen ALIAS jr_svggen)

# Configure public include directories
target_include_directories(jr_svggen
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Depend on QR code generator module
target_link_libraries(jr_svggen PUBLIC jr::qrcodegen)

# Require C++20 feature set for this target
target_compile_features(jr_svggen PUBLIC cxx_std_20)

# Target warning flags
if(MSVC)
    target_compile_options(jr_svggen PRIVATE /W4)
else()
    target_compile_options(jr_svggen PRIVATE -Wall -Wextra -Wpedantic)
endif()