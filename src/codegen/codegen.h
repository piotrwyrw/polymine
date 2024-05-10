#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>

#include "../common/ast.h"

struct codegen {
        struct astnode *program;
        struct astnode *stuff;

        FILE *out;

        // Temporary stuff for code generation and keeping track of state
        size_t param_count;
        size_t param_no;
};

void codegen_init(struct codegen *, struct astnode *, struct astnode *, FILE *);

void gen_generate(struct codegen *);

void gen_any(struct codegen *, struct astnode *);

void gen_resolve(struct codegen *, struct astnode *);

void gen_if(struct codegen *, struct astnode *, size_t);

void gen_include(struct codegen *, struct astnode *);

void gen_type(struct codegen *, struct astdtype *);

void gen_type_definition(struct codegen *, struct astnode *);

void gen_function_definition(struct codegen *, struct astnode *);

void gen_variable_declaration(struct codegen *, struct astnode *);

void gen_assignment(struct codegen *, struct astnode *);

void gen_expression(struct codegen *, struct astnode *);

#endif