#!/usr/bin/bash

TARGET="./target/debug"
CXXFLAGS="-std=c++20 -stdlib=libc++ -g -O0 -I./tomlplusplus/include -fprebuilt-module-path=$TARGET"
LDFLAGS="-stdlib=libc++"
mkdir -p $TARGET
clang++ $CXXFLAGS -x c++-module -c src/arguments.cpp -o $TARGET/arguments.o -fmodule-output \
&& clang++ $CXXFLAGS -c src/shinobi_parser.cpp -o $TARGET/shinobi_parser.o \
&& clang++ $CXXFLAGS -c src/main.cpp -o $TARGET/main.o \
&& clang++ $LDFLAGS $TARGET/main.o $TARGET/arguments.o $TARGET/shinobi_parser.o -o $TARGET/shinobi

