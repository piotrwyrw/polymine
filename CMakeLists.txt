cmake_minimum_required(VERSION 3.27)
project(polymine C)
set(CMAKE_C_STANDARD 90)
set(CMAKE_BUILD_TYPE Debug)
add_executable(polymine src/main.c src/common/ast.c src/common/io.c src/syntax/lexer.c src/syntax/parser.c
        src/common/util.h
        src/common/util.c
        src/syntax/syntax.c
        src/syntax/syntax.h
        src/semantics/semantics.h
        src/semantics/semantics.c
        src/semantics/semutil.h
        src/semantics/semutil.c
        src/codegen/codegen.h
        src/codegen/codegen.c
)