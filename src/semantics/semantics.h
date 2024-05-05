#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "../common/ast.h"

#ifndef FUNCTION_ID
#define FUNCTION_ID(id) ((id) != NULL ? (id) : "<anonymous function>")
#endif

struct semantics;

_Bool analyze_program(struct semantics *, struct astnode *);

_Bool analyze_compound(struct semantics *, struct astnode *);

_Bool analyze_any(struct semantics *, struct astnode *);

_Bool analyze_if(struct semantics *, struct astnode *);

_Bool analyze_variable_declaration(struct semantics *, struct astnode *);

_Bool analyze_function_definition(struct semantics *, struct astnode *);

_Bool analyze_capture_group(struct semantics *, struct astnode *);

_Bool analyze_resolve(struct semantics *, struct astnode *);

_Bool analyze_include(struct semantics *, struct astnode *);

_Bool analyze_assignment(struct semantics *, struct astnode *);

struct astdtype *analyze_expression(struct semantics *, struct astnode *, _Bool *);

struct astdtype *analyze_binary_expression(struct semantics *, struct astnode *, _Bool *);

struct astdtype *analyze_variable_use(struct semantics *, struct astnode *, _Bool *);

struct astdtype *analyze_atom(struct semantics *, struct astnode *, _Bool *);

struct astdtype *analyze_function_call(struct semantics *, struct astnode *);

#endif