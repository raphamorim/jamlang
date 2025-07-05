cmake_minimum_required(VERSION 3.10)
project(JamTests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add test executable (no LLVM dependencies needed for external tests)
add_executable(jam_tests 
    external_tests.cpp 
    test_print_functions.cpp
)