cmake_minimum_required(VERSION 3.20)

project(DiagnosticStressTest LANGUAGES CXX)

# DiagnosticStressTest intentionally contains compile errors.
# This project is for inspecting compiler diagnostics and terminal colors,
# not for producing a successful executable.

add_executable(DiagnosticStressTest DiagnosticStressTest.cpp)

############ FOLLOWING NOT REQUIRED WHEN WINDEV DOSKEYS ARE USED

#[=[
target_compile_features(DiagnosticStressTest PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(DiagnosticStressTest PRIVATE
        /W4
        /FC
        /diagnostics:caret
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(DiagnosticStressTest PRIVATE
        -Wall
        -Wextra
        -Wconversion
        -fcolor-diagnostics
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(DiagnosticStressTest PRIVATE
        -Wall
        -Wextra
        -Wconversion
        -fdiagnostics-color=always
    )
endif()

# CMake may add compiler-specific diagnostic color flags for supported
# GNU/Clang integrations. This does not make native MSVC diagnostics colored.
set(CMAKE_COLOR_DIAGNOSTICS ON)
]=]

