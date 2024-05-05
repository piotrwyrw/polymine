#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#include "../common/ast.h"

struct parser {
        struct lxtok current;
        struct lxtok next;
        struct lexer *lx;
        struct astnode *block;

        // A compound node. This starts off as a way for the parser to store the data
        // types it creates (parses), but is later repurposed by the semantic analysis stage
        // as well as the code generators for storing lambda (ordering) as well as types.
        struct astnode *types;
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
