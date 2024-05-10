#include "common/io.h"
#include "syntax/lexer.h"
#include "syntax/parser.h"
#include "syntax/syntax.h"
#include "semantics/semutil.h"
#include "common/util.h"
#include "codegen/codegen.h"

#include <stdio.h>

void print_license()
{
        printf("Polymine Copyright (C) 2024  Piotr K. Wyrwas\n"
               "This program comes with ABSOLUTELY NO WARRANTY.\n"
               "This is free software, and you are welcome to \nredistribute it "
               "under the terms of the GNU GPL license\n\n");
}

int main(void)
{
//        print_license();

        struct input_handle handle = empty_input_handle;
        if (!input_read("input.poly", &handle)) {
                printf("Could not load input file.\n");
                return 0;
        }

        struct lexer lex;
        lexer_init(&lex, &handle);

        struct parser p;
        parser_init(&p, &lex);

        struct astnode *node = parse(&p);

        if (!node) {
                printf("-- Parsing failed --\n");
                goto syntax_error;
        }

        struct semantics sem;
        semantics_init(&sem, p.types);

        if (!analyze_program(&sem, node)) {
                printf("-- Semantic analysis failed --\n");
                goto semantics_error;
        }

        ast_print(node, 0);

        // --- Code generation

        FILE *output = fopen("output.c", "wa");

        struct codegen gen;
        codegen_init(&gen, node, sem.stuff, output);
        gen_generate(&gen);

        fclose(output);

        // ---

        semantics_error:

        astnode_free(node);

        semantics_free(&sem);

        syntax_error:

        parser_free(&p);

        input_free(&handle);

        return 0;
}
