#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "ast.h"

struct astnode *find_symbol(char *, struct astnode *);

void put_symbol(struct astnode *, struct astnode *);

_Bool analyze_program(struct astnode *);

_Bool analyze_compound(struct astnode *);

_Bool analyze_any(struct astnode *);

_Bool analyze_variable_declaration(struct astnode *);

_Bool analyze_binary_expression(struct astnode *);

_Bool analyze_variable_use(struct astnode *);

#endif