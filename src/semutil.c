#include "semutil.h"

#include <string.h>
#include <math.h>

void semantics_init(struct semantics *sem)
{
        sem->int8 = astdtype_builtin(BUILTIN_INT8);
        sem->int16 = astdtype_builtin(BUILTIN_INT16);
        sem->int32 = astdtype_builtin(BUILTIN_INT32);
        sem->int64 = astdtype_builtin(BUILTIN_INT64);
        sem->byte = astdtype_builtin(BUILTIN_GENERIC_BYTE);
        sem->_double = astdtype_builtin(BUILTIN_DOUBLE);
}

void semantics_free(struct semantics *sem)
{
        astdtype_free(sem->int8);
        astdtype_free(sem->int16);
        astdtype_free(sem->int32);
        astdtype_free(sem->int64);
        astdtype_free(sem->byte);
        astdtype_free(sem->_double);
}

static struct astnode *filter_variable(char *id, struct astnode *node)
{
        if (node->type != NODE_VARIABLE_SYMBOL)
                return NULL;

        if (strcmp(node->variable_symbol.identifier, id) != 0)
                return NULL;

        return node;
}

struct astnode *find_symbol(char *id, struct astnode *block)
{
        if (block->type != NODE_BLOCK) {
                printf("find_symbol(%s): 'block' is a %s!\n", id, nodetype_string(block->type));
                return NULL;
        }

        // Traverse the tree upwards to find the first block that contains said symbol
        struct astnode *b = block;
        struct astnode *symbol;

        while (b != NULL) {
                if ((symbol = astnode_compound_foreach(block->block.symbols, id, (void *) filter_variable)))
                        return symbol;

                b = b->super;
        }

        return NULL;
}

void put_symbol(struct astnode *block, struct astnode *symbol)
{
        astnode_push_compound(block->block.symbols, symbol);
}

_Bool compare_types(struct astdtype *a, struct astdtype *b)
{
        if (a->type != ASTDTYPE_BUILTIN || b->type != ASTDTYPE_BUILTIN)
                return false;

        if (a->builtin.datatype == b->builtin.datatype)
                return true;

        return false;
}

size_t quantify_type_size(struct astdtype *type)
{
        if (type->type == ASTDTYPE_POINTER)
                return sizeof(void *);

        if (type->type != ASTDTYPE_BUILTIN)
                return 0;

        switch (type->builtin.datatype) {
                case BUILTIN_INT64:
                        return 64/8;
                case BUILTIN_INT32:
                        return 32/8;
                case BUILTIN_INT16:
                        return 16/8;
                case BUILTIN_INT8:
                        return 8/8;
                case BUILTIN_CHAR:
                case BUILTIN_GENERIC_BYTE:
                        return sizeof(char);
                case BUILTIN_DOUBLE:
                        return sizeof(double);
                default:
                        return 0;
        }
}

struct astdtype *required_type(struct astdtype *a, struct astdtype *b)
{
        // Nooope!
        if (a->type == ASTDTYPE_CUSTOM || b->type == ASTDTYPE_CUSTOM)
                return NULL;

        // Also nope!
        if (a->type == ASTDTYPE_VOID || b->type == ASTDTYPE_VOID)
                return NULL;

        size_t left, right;

        left = quantify_type_size(a);
        right = quantify_type_size(b);

        if (!left || !right)
                return NULL;

        if (left > right)
                return a;
        else if (right > left)
                return b;

        // It doesn't really matter which type we return here ...
        return a;
}

struct astdtype *required_type_integer(struct semantics *sem, int value)
{
        size_t size =  (size_t) log2((double) value) + 1;

        if (size <= 8/8)
                return sem->int8;
        if (size <= 16/8)
                return sem->int16;
        if (size <= 32/8)
                return sem->int32;
        if (size <= 64/8)
                return sem->int64;

        return NULL;
}