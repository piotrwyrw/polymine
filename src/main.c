#include "io.h"
#include "lexer.h"
#include "parser.h"

#include <stdio.h>

int main(void) {
	struct input_handle handle = empty_input_handle;
	if (!input_read("input", &handle)) {
		printf("Could not load input file.\n");
		return 0;
	}
	
	struct lexer lex;
	lexer_init(&lex, &handle);

	struct parser p;
	parser_init(&p, &lex);

        struct astnode *str = parse_string_literal(&p);
        if (str) {
                printf("%s\n", str->body.string_literal.value);
                astnode_free(str);
        }

        parser_Free(&p);

	input_free(&handle);

	return 0;
}
