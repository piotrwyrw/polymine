#include "ast.h"

#include <stdlib.h>
#include <string.h>

enum builtin_type builtin_from_string(char *str)
{
#define RETURN_IF(val, type) \
                             if (strcmp(val, str) == 0) \
                                return type;

        RETURN_IF("char", BUILTIN_CHAR)
        RETURN_IF("byte", BUILTIN_GENERIC_BYTE)
        RETURN_IF("number", BUILTIN_NUMBER)

#undef RETURN_IF

        return BUILTIN_UNDEFINED;
}

void astdtype_free(struct astdtype *adt)
{
        switch (adt->type) {
                case ASTDTYPE_POINTER:
                        astdtype_free(adt->pointer.to);
                        break;
                case ASTDTYPE_CUSTOM:
                        free(adt->custom.name);
                        break;
                default:
                        break;
        }

        free(adt);
}

struct astdtype *astdtype_generic(enum astdtype_type adtType)
{
        struct astdtype *adt = malloc(sizeof(struct astdtype));
        adt->type = adtType;
        return adt;
}

struct astdtype *astdtype_pointer(struct astdtype *adt)
{
        struct astdtype *wrapper = astdtype_generic(ASTDTYPE_POINTER);
        wrapper->pointer.to = adt;
        return wrapper;
}

struct astdtype *astdtype_builtin(enum builtin_type builtin)
{
        struct astdtype *wrapper = astdtype_generic(ASTDTYPE_BUILTIN);
        wrapper->builtin.datatype = builtin;
        return wrapper;
}

struct astdtype *astdtype_custom(char *name)
{
        struct astdtype *wrapper = astdtype_generic(ASTDTYPE_CUSTOM);
        wrapper->custom.name = strdup(name);
        return wrapper;
}


const char *nodetype_string(enum nodetype type)
{
        switch (type) {
#define AUTO(type) case type: return #type;
                AUTO(NODE_BLOCK)
                AUTO(NODE_PROGRAM)
                AUTO(NODE_NUMBER_LITERAL)
                AUTO(NODE_STRING_LITERAL)
#undef AUTO
                default:
                        return "[???]";
        }
}

enum binaryop bop_from_lxtype(enum lxtype type)
{
        switch (type) {
                case LX_PLUS:
                        return BOP_ADD;
                case LX_MINUS:
                        return BOP_SUB;
                case LX_SLASH:
                        return BOP_DIV;
                case LX_ASTERISK:
                        return BOP_MUL;
                default:
                        return BOP_UNKNOWN;
        }
}

const char *binaryop_string(enum binaryop op)
{
#define AUTO(type) case type: return #type;
        switch (op) {
                AUTO(BOP_UNKNOWN);
                AUTO(BOP_ADD)
                AUTO(BOP_SUB)
                AUTO(BOP_MUL)
                AUTO(BOP_DIV)
        }
#undef AUTO
}

void astnode_free(struct astnode *node)
{
        if (!node)
                return;

        switch (node->type) {
                case NODE_PROGRAM:
                        astnode_free(node->program.block);
                        break;
                case NODE_BLOCK:
                        astnode_free_block(node);
                        return;
                case NODE_STRING_LITERAL:
                        free(node->string_literal.value);
                        break;
                case NODE_BINARY_OP:
                        astnode_free(node->binary.left);
                        astnode_free(node->binary.right);
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

struct astnode *astnode_nothing(size_t line)
{
        return astnode_generic(NODE_NOTHING, line);
}

struct astnode *astnode_empty_program()
{
        struct astnode *node = astnode_generic(NODE_PROGRAM, 0);
        node->program.block = NULL;
        return node;
}

struct astnode *astnode_empty_block(size_t line)
{
        struct astnode *node = astnode_generic(NODE_BLOCK, line);

        node->block.max_count = NODE_ARRAY_INCREMENT;
        node->block.count = 0;
        node->block.nodes = calloc(node->block.max_count, sizeof(struct astnode *));

        return node;
}

_Bool astnode_push_block(struct astnode *block, struct astnode *node)
{
        _Bool resize = false;

        // Resize the array if needed
        if (block->block.count >= block->block.max_count) {
                block->block.nodes = realloc(block->block.nodes, sizeof(struct astnode *) *
                                                                 (block->block.max_count += NODE_ARRAY_INCREMENT));
                resize = true;
        }

        block->block.nodes[block->block.count++] = node;
        return resize;
}

void astnode_free_block(struct astnode *block)
{
        for (size_t i = 0; i < block->block.count; i++) {
                astnode_free(block->block.nodes[i]);
        }

        free(block->block.nodes);
        free(block);
}

struct astnode *astnode_number_literal(size_t line, double value)
{
        struct astnode *node = astnode_generic(NODE_NUMBER_LITERAL, line);
        node->number_literal.value = value;
        return node;
}

struct astnode *astnode_string_literal(size_t line, char *value)
{
        struct astnode *node = astnode_generic(NODE_STRING_LITERAL, line);
        node->string_literal.value = strdup(value);
        node->string_literal.length = strlen(value);
        return node;
}

struct astnode *astnode_binary(size_t line, struct astnode *left, struct astnode *right, enum binaryop op)
{
        struct astnode *node = astnode_generic(NODE_BINARY_OP, line);
        node->binary.left = left;
        node->binary.right = right;
        node->binary.op = op;
        return node;
}