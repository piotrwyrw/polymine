cmake_minimum_required(VERSION 3.27)
project(polymine C)
set(CMAKE_BUILD_TYPE Debug)
add_executable(polymine src/main.c src/ast.c src/io.c src/lexer.c src/parser.c
        src/util.h
        src/util.c
        src/syntax.c
        src/syntax.h
        src/semantics.h
)