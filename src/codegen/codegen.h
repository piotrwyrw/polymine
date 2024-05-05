#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>

#include "../common/ast.h"

struct codegen {
        struct astnode *program;
        struct astnode *stuff;

        FILE *out;
};

void codegen_init(struct codegen *, struct astnode *, struct astnode *, FILE *);

void gen_generate(struct codegen *);

void gen_any(struct codegen *, struct astnode *);

void gen_include(struct codegen *, struct astnode *);

void gen_type(struct codegen *, struct astdtype *, char *);

void gen_function_definition(struct codegen *, struct astnode *);

#endif