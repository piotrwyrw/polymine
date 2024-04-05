//
// Created by Piotr Krzysztof Wyrwas on 24.03.24.
//

#ifndef UTIL_H
#define UTIL_H

#include "ast.h"

int char_to_digit(char c);

double string_to_float(char *);

int64_t string_to_integer(char *);

char *repeat(char, size_t);

void ast_print(struct astnode *, size_t);

#endif
