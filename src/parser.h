#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#include "ast.h"

struct parser {
	struct lxtok current;
	struct lxtok next;
	struct lexer *lx;
        struct astnode *block;
	size_t line;
};

// This basically tells us where we need to display the error message.
// NO_MATCH tells us that the parser couldn't find an appropriate parser endpoint, while
// SYNTAX_ERROR states that parsing was attempted, and subsequently failed with a syntax error
// that was already displayed somewhere else.
enum pstatus {
        PARSER_NO_MATCH,
        PARSER_SYNTAX_ERROR
};

void parser_init(struct parser *, struct lexer *);

void parser_free(struct parser *);

void parser_advance(struct parser *);

#endif
