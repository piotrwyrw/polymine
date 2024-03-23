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

void parser_init(struct parser *, struct lexer *);

void parser_Free(struct parser *);

void parser_advance(struct parser *);

struct astdtype *parse_type(struct parser *);

struct astnode *parse_string_literal(struct parser *);

#endif
