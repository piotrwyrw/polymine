#include "io.h"
#include "lexer.h"
#include "parser.h"
#include "syntax.h"
#include "util.h"
#include "semantics.h"

#include <stdio.h>

int main(void)
{
        struct input_handle handle = empty_input_handle;
        if (!input_read("input.int", &handle)) {
                printf("Could not load input file.\n");
                return 0;
        }

        struct lexer lex;
        lexer_init(&lex, &handle);

        struct parser p;
        parser_init(&p, &lex);

        struct astnode *node = parse(&p);

        if (node) {
                analyze_program(node);
                ast_print(node, 0);
                astnode_free(node);
        }

        parser_free(&p);

        input_free(&handle);

        return 0;
}
