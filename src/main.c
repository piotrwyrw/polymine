#include "io.h"
#include "lexer.h"
#include "parser.h"
#include "syntax.h"
#include "util.h"

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
        print_license();

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
                ast_print(node, 0);
                astnode_free(node);
        }

        parser_free(&p);

        input_free(&handle);

        return 0;
}
