.PHONY: build
.DEFAULT_GOAL := build

LLVM_CONFIG=$(shell whereis -q llvm-config)

build:
	@echo ${LLVM_CONFIG}
	clang++ -c ./src/main.cpp -o ./jam.o `${LLVM_CONFIG} --cxxflags` -fexceptions
	clang++ -o ./jam.out ./jam.o `${LLVM_CONFIG} --ldflags --libs --libfiles --system-libs`

build-cmake:
	@mkdir -p build
	cd build && cmake ..
	cd build && cmake --build .
	@echo "Build complete! The compiler executable is located at: $(pwd)/simple_compiler"
