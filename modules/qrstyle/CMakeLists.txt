cmake_minimum_required(VERSION 3.20)

add_library(jr_qrstyle STATIC
    src/QrStyle.cpp
)
add_library(jr::qrstyle ALIAS jr_qrstyle)

target_include_directories(jr_qrstyle
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(jr_qrstyle PUBLIC jr::qrcodegen)
target_compile_features(jr_qrstyle PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(jr_qrstyle PRIVATE /W4 /permissive-)
else()
    target_compile_options(jr_qrstyle PRIVATE -Wall -Wextra -Wpedantic)
endif()
