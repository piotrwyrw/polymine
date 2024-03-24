cmake_minimum_required(VERSION 3.27)
project(interpreter C)
set(CMAKE_BUILD_TYPE Debug)
add_executable(interpreter src/main.c src/ast.c src/io.c src/lexer.c src/parser.c
        src/util.h
        src/util.c)