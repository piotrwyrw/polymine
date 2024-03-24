//
// Created by Piotr Krzysztof Wyrwas on 24.03.24.
//

#ifndef SYNTAX_H
#define SYNTAX_H

#include "parser.h"

struct astnode *parse(struct parser *);

struct astnode *parse_block_advanced(struct parser *, _Bool);

struct astnode *parse_block(struct parser *);

struct astnode *parse_nothing(struct parser *);

struct astnode *parse_variable_declaration(struct parser *);

struct astnode *parse_variable_assignment(struct parser *);

struct astdtype *parse_type(struct parser *);

struct astnode *parse_expr(struct parser *);

struct astnode *parse_additive_expr(struct parser *);

struct astnode *parse_multiplicative_expr(struct parser *);

struct astnode *parse_atom(struct parser *);

struct astnode *parse_number(struct parser *);

struct astnode *parse_string_literal(struct parser *);


#endif
