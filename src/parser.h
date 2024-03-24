#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#include "ast.h"

struct parser {
	struct lxtok current;
	struct lxtok next;
	struct lexer *lx;
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

struct astnode *parser_parse(struct parser *);

struct astnode *parse_block_advanced(struct parser *, _Bool);

struct astnode *parse_block(struct parser *);

struct astnode *parse_nothing(struct parser *);

struct astdtype *parse_type(struct parser *);

struct astnode *parse_expr(struct parser *);

struct astnode *parse_additive_expr(struct parser *);

struct astnode *parse_multiplicative_expr(struct parser *);

struct astnode *parse_atom(struct parser *);

struct astnode *parse_number(struct parser *);

struct astnode *parse_string_literal(struct parser *);

#endif
