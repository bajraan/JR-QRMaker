cmake_minimum_required(VERSION 3.20)

# Define static library target for QR code generator core
add_library(jr_qrcodegen STATIC
    src/qrcodegen.cpp
)

# Create namespaced alias target for best CMake practices
add_library(jr::qrcodegen ALIAS jr_qrcodegen)

# Configure public include directories
target_include_directories(jr_qrcodegen
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Require C++20 feature set for this target
target_compile_features(jr_qrcodegen PUBLIC cxx_std_20)

# Target warning flags
if(MSVC)
    target_compile_options(jr_qrcodegen PRIVATE /W4)
else()
    target_compile_options(jr_qrcodegen PRIVATE -Wall -Wextra -Wpedantic)
endif()