#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "ast.h"

struct semantics;

_Bool analyze_program(struct semantics *, struct astnode *);

_Bool analyze_compound(struct semantics *, struct astnode *);

_Bool analyze_any(struct semantics *, struct astnode *);

_Bool analyze_variable_declaration(struct semantics *, struct astnode *);

_Bool analyze_function_definition(struct semantics *, struct astnode *);

_Bool analyze_resolve(struct semantics *, struct astnode *);

struct astdtype *analyze_expression(struct semantics *, struct astnode *);

struct astdtype *analyze_binary_expression(struct semantics *, struct astnode *);

struct astdtype *analyze_variable_use(struct semantics *, struct astnode *);

struct astdtype *analyze_atom(struct semantics *, struct astnode *);

#endif