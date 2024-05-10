#include "semutil.h"

#include <string.h>
#include <math.h>

void semantics_init(struct semantics *sem, struct astnode *types, struct astnode *program)
{
        sem->int8 = astdtype_builtin(BUILTIN_INT8);
        sem->int16 = astdtype_builtin(BUILTIN_INT16);
        sem->int32 = astdtype_builtin(BUILTIN_INT32);
        sem->int64 = astdtype_builtin(BUILTIN_INT64);
        sem->byte = astdtype_builtin(BUILTIN_GENERIC_BYTE);
        sem->_double = astdtype_builtin(BUILTIN_DOUBLE);
        sem->_void = astdtype_void();
        sem->string = astdtype_string_type();
        sem->stuff = types;
        sem->symbol_counter = 0;
        sem->pristine = true;

        sem->program = program;

        semantics_new_include(sem, "inttypes.h");
}

void semantics_free(struct semantics *sem)
{
        astdtype_free(sem->int8);
        astdtype_free(sem->int16);
        astdtype_free(sem->int32);
        astdtype_free(sem->int64);
        astdtype_free(sem->byte);
        astdtype_free(sem->_double);
        astdtype_free(sem->_void);
        astdtype_free(sem->string);
}

static struct astnode *filter_symbol(char *id, struct astnode *node)
{
        if (!id)
                return NULL;

        if (node->type != NODE_SYMBOL)
                return NULL;

        if (strcmp(node->symbol.identifier, id) != 0)
                return NULL;

        return node;
}

struct astnode *custom_traverse(void *param, void *(*callback)(void *, struct astnode *), struct astnode *block, enum traverse_params domain)
{
        if (block->type != NODE_BLOCK) {
                printf("custom_traverse(..): Block is a %s!\n", nodetype_string(block->type));
                return NULL;
        }

        struct astnode *b = block;
        struct astnode *node;

        while (b != NULL) {
                if ((node = astnode_compound_foreach((domain & TRAVERSE_SYMBOLS) ? b->block.symbols : b->block.nodes,
                                                     param, callback)))
                        return node;

                if (domain & TRAVERSE_HALT_NESTED && b->holder && b->holder->type == NODE_FUNCTION_DEFINITION) {
                        if (!(domain & TRAVERSE_GLOBAL_REGARDLESS))
                                return NULL;

                        // Traverse the tree up all the way up until the global scope
                        while (true) {
                                if (!b->holder)
                                        return NULL;

                                if (b->holder->type == NODE_PROGRAM)
                                        break;

                                if (b->super)
                                        b = b->super;
                        }

                        return custom_traverse(param, callback, b, domain);
                }

                b = b->super;
        }

        return NULL;
}

struct astnode *find_symbol(char *id, struct astnode *block)
{
        return custom_traverse(id, (void *) filter_symbol, block,
                               TRAVERSE_SYMBOLS | TRAVERSE_HALT_NESTED | TRAVERSE_GLOBAL_REGARDLESS);
}

struct astnode *find_symbol_shallow(char *id, struct astnode *block)
{
        return astnode_compound_foreach(block->block.symbols, id, (void *) filter_symbol);
}

struct astnode *find_enclosing_function(struct astnode *block)
{
        if (block->type != NODE_BLOCK) {
                printf("find_enclosing_function(..): Block is a %s!\n", nodetype_string(block->type));
                return NULL;
        }

        struct astnode *b = block;

        while (b != NULL) {
                if (b->holder && b->holder->type == NODE_FUNCTION_DEFINITION)
                        return b->holder;

                b = b->super;
        }

        return NULL;
}


// Find any control structures (IFs, WHILEs) on the way to the enclosing function that affect
// the potential reachability of a resolve statement
struct astnode *find_uncertain_reachability_structures(struct astnode *block)
{
        if (block->type != NODE_BLOCK) {
                printf("find_uncertain_reachability_structures(..): Block is a %s!\n", nodetype_string(block->type));
                return NULL;
        }

        struct astnode *b = block;

        while (b != NULL) {
                if (b->holder && b->holder->type == NODE_FUNCTION_DEFINITION)
                        return NULL;

                if (b->holder->type == NODE_IF)
                        return b->holder;

                b = b->super;
        }

        return NULL;
}

void put_symbol(struct astnode *block, struct astnode *symbol)
{
        astnode_push_compound(block->block.symbols, symbol);
}

_Bool is_uppermost_block(struct astnode *block)
{
        if (block->super)
                return false;

        return true;
}

/**
 * @param pointer - The types must be an EXACT match
 */
static _Bool types_compatible_advanced(struct astdtype *destination, struct astdtype *source, _Bool pointer)
{
        if (destination->type != source->type)
                return false;

        if (destination->type == ASTDTYPE_POINTER)
                return types_compatible_advanced(destination->pointer.to, source->pointer.to, true);

        if (destination->type == ASTDTYPE_VOID)
                return true;

        if (destination->type == ASTDTYPE_COMPLEX && strcmp(destination->complex.name, source->complex.name) == 0)
                return true;

#define XOR(a, b) (((a) && !(b)) || (!(a) && (b)))

        if (destination->type == ASTDTYPE_BUILTIN) {
                if (destination->builtin.datatype != source->builtin.datatype && pointer)
                        return false;

                if (XOR(source->builtin.datatype == BUILTIN_STRING, destination->builtin.datatype == BUILTIN_STRING))
                        return false;

                if (XOR(source->builtin.datatype == BUILTIN_DOUBLE, destination->builtin.datatype == BUILTIN_DOUBLE))
                        return false;

                if ((quantify_type_size(destination) >= quantify_type_size(source)))
                        return true;
        }

#undef XOR

        return false;
}

_Bool types_compatible(struct astdtype *destination, struct astdtype *source)
{
        return types_compatible_advanced(destination, source, false);
}

size_t quantify_type_size(struct astdtype *type)
{
        if (type->type == ASTDTYPE_POINTER)
                return sizeof(void *);

        if (type->type != ASTDTYPE_BUILTIN)
                return 0;

        switch (type->builtin.datatype) {
                case BUILTIN_INT64:
                        return 64 / 8;
                case BUILTIN_INT32:
                        return 32 / 8;
                case BUILTIN_INT16:
                        return 16 / 8;
                case BUILTIN_INT8:
                        return 8 / 8;
                case BUILTIN_CHAR:
                case BUILTIN_GENERIC_BYTE:
                        return sizeof(char);
                case BUILTIN_DOUBLE:
                        return sizeof(double);
                case BUILTIN_STRING:
                        return 64 / 8;
                default:
                        return 0;
        }
}

struct astdtype *required_type(struct astdtype *a, struct astdtype *b)
{
        // Nooope!
        if (a->type == ASTDTYPE_COMPLEX || b->type == ASTDTYPE_COMPLEX)
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
        size_t size = ((size_t) log2(value) + 1);

        if (size <= 8)
                return sem->int8;
        if (size <= 16)
                return sem->int16;
        if (size <= 32)
                return sem->int32;
        if (size <= 64)
                return sem->int64;

        return NULL;
}

struct astdtype *semantics_new_type(struct semantics *sem, struct astdtype *type)
{
        astnode_push_compound(sem->stuff, astnode_data_type(type));
        return type;
}

void semantics_new_function(struct semantics *sem, struct astnode *function)
{
        struct astnode *gf = astnode_generated_function(function, sem->symbol_counter++);
        astnode_push_compound(sem->stuff, gf);
        function->function_def.generated = gf;
}

void semantics_new_include(struct semantics *sem, char *path)
{
        struct astnode *include = astnode_include(0, NULL, path);
        astnode_push_compound(sem->stuff, include);
}

_Bool has_attribute(struct astnode *compound, char const *iden)
{
        for (size_t i = 0; i < compound->node_compound.count; i++)
                if (strcmp(compound->node_compound.array[i]->attribute.identifier, iden) == 0)
                        return true;

        return false;
}

struct astnode *symbol_conflict(char *id, struct astnode *node)
{
        struct astnode *symbol;

        // Node: Functions may not be shadowed to avoid confusion
        if ((symbol = find_symbol(id, node->super)) && symbol->super == node->super) {
                printf("The symbol '%s' is already defined - Redefinition attempted on line %ld. Previous definition on line %ld as a %s.\n",
                       id, node->line, symbol->line,
                       symbol_type_humanstr(symbol->symbol.symtype));
                return symbol;
        }

        return NULL;
}