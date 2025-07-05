# Jam Programming Language

Jam is a general-purpose programming language. Jam strives to give developers the same level of freedom and control as C, while introducing modern safety features to reduce common programming pitfalls. Jam is designed for those who want to write bare-metal, high-performance software without being constrained by heavy abstractions — but also without being left unguarded against memory bugs, undefined behavior, and other issues.

## Features

- **Go-like syntax**: Clean, readable syntax with familiar control flow
- **Multiple integer types**: u8, u16, u32, i8, i16, i32 with explicit sizing
- **String literals and slices**: UTF-8 string support with `str` type
- **Boolean type**: Native `bool` type with `true`/`false` values
- **Control flow**: if/else statements, for loops, while loops with break/continue
- **Functions**: First-class functions with explicit parameter and return types
- **JIT execution**: Run code directly with `--run` flag for rapid development

## Installation

### Quick Install (Recommended)

```bash
# Clone the repository
git clone <repository-url>
cd jamlang

# Run the installation script
./install.sh
```

### Manual Installation Options

#### Option 1: Using CMake (Recommended)
```bash
# Build and install system-wide
make cmake-install

# Or build first, then install
./build.sh
cd build && sudo make install
```

#### Option 2: Using Make
```bash
# Build and install manually
make install

# Or just build locally
make build
```

#### Option 3: Local Build Only
```bash
# Build without installing
./build.sh

# Use the local executable
./build/jam your-program.jam
```

### Uninstallation

```bash
# If installed via install.sh or cmake
./uninstall.sh

# If installed via make
make uninstall

# If installed via CMake
make cmake-uninstall
```

## Build Requirements

- **LLVM >= 20**: For code generation and JIT execution
- **CMake >= 3.10**: For build system
- **Clang**: C++ compiler with C++20 support

### Installing Dependencies

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

#### Arch Linux
```bash
sudo pacman -S llvm cmake clang
```

## Usage

Once installed, you can use the `jam` compiler from anywhere:

```bash
# Show help
jam --help

# Compile to binary
jam program.jam
./output

# Run directly (JIT execution)
jam --run program.jam

# Example programs
jam --run tui.jam                    # Run the TUI demo
jam tests/unit/test_u8.jam          # Compile a test file
```

## Quick Start

Create a simple "Hello, World!" program:

```jam
// hello.jam
fn main() -> u32 {
    println("Hello, World!");
    return 0;
}
```

Run it:
```bash
jam --run hello.jam
```

## Language Examples

### Basic Types and Variables
```jam
fn main() -> u32 {
    const name: str = "Jam";
    const version: u8 = 1;
    var is_awesome: bool = true;
    
    println("Welcome to Jam!");
    return 0;
}
```

### Control Flow
```jam
fn fibonacci(n: u32) -> u32 {
    if (n <= 1) {
        return n;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

fn main() -> u32 {
    for i in 0:10 {
        const result: u32 = fibonacci(i);
        println("fib result");
    }
    return 0;
}
```

### Loops and Break/Continue
```jam
fn main() -> u32 {
    for i in 0:10 {
        if (i == 5) {
            continue;
        }
        if (i == 8) {
            break;
        }
        println("Iteration");
    }
    return 0;
}
```

## Development

### Building from Source
```bash
# Clone and build
git clone <repository-url>
cd jamlang
./build.sh

# Run tests
./run_all_tests.sh

# Install for development
make cmake-install
```

### Project Structure
```
jamlang/
├── src/main.cpp           # Compiler source code
├── tests/                 # Test suite
│   ├── unit/             # Jam language tests
│   └── cpp/              # C++ unit tests
├── build.sh              # Build script
├── install.sh            # Installation script
├── uninstall.sh          # Uninstallation script
├── Makefile              # Make targets
├── CMakeLists.txt        # CMake configuration
└── README.md             # This file
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run the test suite: `./run_all_tests.sh`
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
