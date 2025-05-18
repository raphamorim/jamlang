cmake_minimum_required(VERSION 3.10)
project(Jam)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM package
find_package(LLVM REQUIRED CONFIG)

# Print LLVM information
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Include LLVM directories
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Add compiler executable
add_executable(jam src/main.cpp)

# Get proper link libraries for LLVM
llvm_map_components_to_libnames(llvm_libs
  Core
  ExecutionEngine
  Interpreter
  MC
  Support
  nativecodegen
  OrcJIT
  native
)

# Link against LLVM libraries
target_link_libraries(jam ${llvm_libs})

# Installation rules
install(TARGETS jam DESTINATION bin)
