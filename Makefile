.PHONY: build
.DEFAULT_GOAL := build

LLVM_CONFIG=$(shell whereis -q llvm-config)

build:
	@echo ${LLVM_CONFIG}
	clang++ -c ./src/main.cpp -o ./jam.o `${LLVM_CONFIG} --cxxflags`
	clang++ -o ./jam.out ./jam.o `${LLVM_CONFIG} --ldflags --libs --libfiles --system-libs`
