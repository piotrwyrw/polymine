#include "ast.h"

#include <stdlib.h>
#include <string.h>

const char *nodetype_string(enum nodetype type)
{
        switch (type) {
#define AUTO(type) case type: return #type;
                AUTO(NODE_BLOCK)
                AUTO(NODE_PROGRAM)
                AUTO(NODE_INTEGER_LITERAL)
                AUTO(NODE_DECIMAL_LITERAL)
                AUTO(NODE_STRING_LITERAL)
#undef AUTO()
                default:
                        return "[???]";
        }
}

void astnode_free(struct astnode *node)
{
	if (!node)
		return;

	switch (node->type) {
		case NODE_PROGRAM:
			astnode_free(node);
			break;
		case NODE_BLOCK:
			astnode_free_block(node);
			return;
		case NODE_STRING_LITERAL:
			free(node->body.string_literal.value);
			break;
		default:
			break;
	}

	free(node);
}

struct astnode *astnode_generic(enum nodetype type, size_t line)
{
	struct astnode *node = malloc(sizeof(struct astnode));	
	node->type = type;
	node->line = line;
	return node;
}

struct astnode *astnode_empty_program()
{
	struct astnode *node = astnode_generic(NODE_PROGRAM, 0);
	node->body.program.block = astnode_empty_block(0);
	return node;
}

struct astnode *astnode_empty_block(size_t line)
{
	struct astnode *node = astnode_generic(NODE_BLOCK, line);

	node->body.block.max_count = NODE_ARRAY_INCREMENT;
	node->body.block.count = 0;
	node->body.block.nodes = calloc(node->body.block.max_count, sizeof(struct astnode));

	return node;
}

_Bool astnode_push_block(struct astnode *program, struct astnode *node)
{
	_Bool resize = false;

	// Resize the array if needed
	if (program->body.block.count >= program->body.block.max_count) {
		program->body.block.nodes = realloc(program->body.block.nodes, sizeof(struct astnode *) * (program->body.block.max_count += NODE_ARRAY_INCREMENT));
		resize = true;
	}

	program->body.block.nodes[program->body.block.count ++] = node;
	return resize;
}

void astnode_free_block(struct astnode *block)
{
	for (size_t i = 0; i < block->body.block.count; i ++) {
		astnode_free(block->body.block.nodes[i]);
	}

	free(block);
}

struct astnode *astnode_integer_literal(size_t line, int value)
{
	struct astnode *node = astnode_generic(NODE_INTEGER_LITERAL, line);
	node->body.integer_literal.value = value;
	return node;
}

struct astnode *astnode_decimal_literal(size_t line, double value)
{
	struct astnode *node = astnode_generic(NODE_DECIMAL_LITERAL, line);
	node->body.decimal_literal.value = value;
	return node;
}

struct astnode *astnode_string_literal(size_t line, char *value)
{
	struct astnode *node = astnode_generic(NODE_STRING_LITERAL, line);
	node->body.string_literal.value = strdup(value);
	node->body.string_literal.length = strlen(value);
	return node;
}