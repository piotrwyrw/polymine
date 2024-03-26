//
// Created by Piotr Krzysztof Wyrwas on 24.03.24.
//

#ifndef UTIL_H
#define UTIL_H

#include "ast.h"

double char_to_digit(char c);

double string_to_number(char *);

char *repeat(char, size_t);

void ast_print(struct astnode *, size_t);

void type_print(struct astdtype *);

#endif
