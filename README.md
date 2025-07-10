# Jam Programming Language

Jam is a systems programming language designed to provide developers with the control and performance characteristics of C while incorporating modern safety features to mitigate common programming errors. The language targets developers who require bare-metal performance and direct hardware access without sacrificing code reliability and maintainability.

## Abstract

This implementation presents a compiler for the Jam programming language, featuring LLVM-based code generation, just-in-time execution capabilities, and a comprehensive type system. The compiler supports multiple integer types with explicit bit-width specifications, UTF-8 string handling, and modern control flow constructs while maintaining compatibility with low-level system programming requirements.

## Language Features

### Type System
- **Integer Types**: Explicit bit-width integers (u8, u16, u32, i8, i16, i32) with well-defined overflow behavior
- **String Type**: UTF-8 compliant string literals with slice-based representation
- **Boolean Type**: Native boolean type with explicit true/false semantics
- **Type Safety**: Compile-time type checking with explicit type annotations

### Control Flow
- **Conditional Statements**: if/else constructs with boolean expression evaluation
- **Iteration Constructs**: for loops with range syntax and while loops
- **Control Transfer**: break and continue statements for loop control
- **Function Definitions**: First-class functions with explicit parameter and return type specifications

### Execution Model
- **Ahead-of-Time Compilation**: Traditional compilation to native machine code
- **Just-in-Time Execution**: Direct execution via LLVM JIT for development workflows

## Installation and Build Requirements

### Prerequisites
- **LLVM Framework**: Version 20 or higher for code generation and optimization
- **CMake**: Version 3.10 or higher for build system management
- **Clang Compiler**: C++20 compliant compiler for host compilation

### Installation Procedures

#### Automated Installation
```bash
git clone <repository-url>
cd jamlang
./install.sh
```

#### Manual Build Process
```bash
# CMake-based build (recommended)
make cmake-install

# Alternative make-based build
make install

# Local development build
./build.sh
```

### Platform-Specific Dependencies

#### macOS
```bash
brew install llvm cmake
```

#### Ubuntu/Debian
```bash
sudo apt-get install llvm-dev cmake clang
```

#### CentOS/RHEL
```bash
sudo yum install llvm-devel cmake clang
```

## Usage and Command-Line Interface

### Basic Operations
```bash
# Display compiler options
jam --help

# Compile source to executable
jam program.jam

# Execute via JIT compilation
jam --run program.jam
```

### Example Programs

#### Basic Program Structure
```jam
fn main() -> u32 {
    println("Hello, World!");
    return 0;
}
```

#### Type System Demonstration
```jam
fn demonstrate_types() -> u32 {
    const byte_value: u8 = 255;
    const word_value: u16 = 65535;
    const dword_value: u32 = 4294967295;
    const flag: bool = true;
    const message: str = "Type system demonstration";
    
    return 0;
}
```

#### Control Flow Examples
```jam
fn fibonacci(n: u32) -> u32 {
    if (n <= 1) {
        return n;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

fn iterative_example() -> u32 {
    for i in 0:10 {
        if (i == 5) {
            continue;
        }
        if (i == 8) {
            break;
        }
        const result: u32 = fibonacci(i);
    }
    return 0;
}
```

## Testing and Validation

### Test Suite Execution
```bash
# Run comprehensive test suite
make test

# Execute individual test categories
./run_tests.sh           # Language-level tests
./tests/cpp/build_and_run.sh  # Compiler unit tests
```

### Test Coverage
The test suite validates:
- Lexical analysis of type annotations and literals
- Syntactic parsing of language constructs
- Semantic analysis and type checking
- LLVM IR generation correctness
- End-to-end compilation pipeline integrity

## Architecture and Implementation

### Compiler Pipeline
1. **Lexical Analysis**: Tokenization of source code with type-aware scanning
2. **Syntactic Analysis**: Recursive descent parsing with error recovery
3. **Semantic Analysis**: Type checking and symbol resolution
4. **Code Generation**: LLVM IR emission with optimization passes
5. **Linking**: Native code generation and executable production

### Project Organization
```
jamlang/
├── src/main.cpp           # Compiler implementation
├── tests/                 # Validation suite
│   ├── unit/             # Language feature tests
│   └── cpp/              # Compiler unit tests
├── build.sh              # Build automation
├── CMakeLists.txt        # Build configuration
└── documentation/        # Language specification
```

## Research and Development

### Contributing Guidelines
1. Fork the repository and create a feature branch
2. Implement changes following existing code conventions
3. Validate modifications using the test suite: `make test`
4. Submit pull request with detailed change description

### Future Research Directions
- Memory management strategies for systems programming
- Optimization techniques for embedded target architectures
- Integration with existing C/C++ codebases
- Performance analysis and benchmarking frameworks

## License and Distribution

This software is distributed under the MIT License. See the LICENSE file for complete terms and conditions.
