# Jam Programming Language - Development Guide

## Build Commands
- **Build compiler**: `./build.sh` (creates `build/jam` executable)
- **Build with Make**: `make` (alternative build using Makefile)
- **Clean build**: `rm -rf build && ./build.sh`

## Installation Commands
- **Quick install**: `./install.sh` (automated installation script)
- **Install with CMake**: `make cmake-install` (recommended method)
- **Install with Make**: `make install` (manual method)
- **Uninstall**: `./uninstall.sh` or `make cmake-uninstall` or `make uninstall`
- **Installation info**: `make info` (show installation paths and options)

## Compiler Usage
- **Compile to binary**: `jam <filename.jam>` (creates `output` executable)
- **Run directly**: `jam --run <filename.jam>` (executes without creating binary)
- **Show help**: `jam --help` (displays usage information)

## Test Commands
- **Run all tests**: `./run_all_tests.sh` (comprehensive test suite)
- **Run Jam tests only**: `./run_tests.sh` (unit tests for .jam files)
- **Run C++ tests only**: `cd tests/cpp && ./build_and_run.sh`
- **Run C++ tests manually**: `cd tests/cpp/build && ./jam_tests`
- **Run single test**: `./build/jam tests/unit/test_u8.jam` (replace with specific test file)

## Code Style Guidelines
- **Language**: C++20 with LLVM integration
- **Headers**: Include copyright header with MIT license
- **Includes**: System headers first, then LLVM headers, then local headers
- **Naming**: snake_case for functions/variables, PascalCase for enums/classes
- **Types**: Use explicit types (u8, u16, u32, bool, str, []T) in Jam code
- **Functions**: Always specify return types with `-> type` syntax
- **Variables**: Use `const` for immutable, `var` for mutable in Jam
- **Strings**: Use `str` for string slices, `"literal"` for string literals
- **Slices**: Use `[]T` syntax for slice types (e.g., `[]u8`, `[]const u8`)
- **Loops**: Use `for i in start:end { ... }` for range loops, `while (condition) { ... }` for conditional loops
- **Control flow**: Use `break` and `continue` statements within loops
- **Formatting**: Follow existing indentation and brace style
- **Error handling**: Use LLVM error handling patterns and std::optional

## Architecture
- Single-file compiler in `src/main.cpp` with lexer, parser, and LLVM codegen
- Tests in `tests/unit/` for Jam language features, `tests/cpp/` for C++ unit tests
- Build system uses CMake with LLVM >= 20 requirement
- **String system**: `str` type for string slices, UTF-8 by default
- **Slice system**: `[]T` types for dynamic arrays (e.g., `[]u8`, `[]i32`)
- **Memory representation**: Slices as `{ptr, len}` structs in LLVM IR
- **Loop system**: For loops with range syntax `for i in start:end`, while loops with conditions
- **Control flow**: Break/continue statements with proper LLVM basic block management