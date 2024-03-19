#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <stdbool.h>

#define NODE_ARRAY_INCREMENT 20

enum nodetype {
	NODE_UNDEFINED = 0,

	NODE_BLOCK,
	NODE_PROGRAM,
	NODE_INTEGER_LITERAL,
	NODE_DECIMAL_LITERAL,
	NODE_STRING_LITERAL
};

const char *nodetype_string(enum nodetype);

struct astnode {
	enum nodetype type;
	size_t line;
	union {
		struct {
			struct astnode *block;
		} program;

		struct {
			struct astnode **nodes;
			size_t max_count;
			size_t count;
		} block;

		struct {
			int value;
		} integer_literal;

		struct {
			double value;
		} decimal_literal;

		struct {
			size_t length;
			char *value;
		} string_literal;
	} body;
};

/* Recursively free a node */
void astnode_free(struct astnode *);

struct astnode *astnode_empty_program();

struct astnode *astnode_empty_block(size_t);

_Bool astnode_push_block(struct astnode *, struct astnode *);

void astnode_free_block(struct astnode *);

struct astnode *astnode_generic(enum nodetype, size_t);

struct astnode *astnode_integer_literal(size_t, int);

struct astnode *astnode_decimal_literal(size_t, double);

struct astnode *astnode_string_literal(size_t, char *);

#endif
