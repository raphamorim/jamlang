# Jam Programming Language

Jam is a general-purpose programming language. Jam strives to give developers the same level of freedom and control as C, while introducing modern safety features to reduce common programming pitfalls. Jam is designed for those who want to write bare-metal, high-performance software without being constrained by heavy abstractions — but also without being left unguarded against memory bugs, undefined behavior, and other issues.

## Build Requirements

- LLVM >= 20
- CMake
- Clang

## Build

```bash
chmod +x build.sh
./build.sh
./build/jam tests/fixtures/8-bit-unsigned-integer/sum.jam
```
