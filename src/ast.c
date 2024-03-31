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
        RETURN_IF("double", BUILTIN_DOUBLE)
        RETURN_IF("int8", BUILTIN_INT8)
        RETURN_IF("int16", BUILTIN_INT16)
        RETURN_IF("int32", BUILTIN_INT32)
        RETURN_IF("int64", BUILTIN_INT64)

#undef RETURN_IF

        return BUILTIN_UNDEFINED;
}

const char *builtin_string(enum builtin_type type)
{
#define AUTO(type) case type: return #type;
        switch (type) {
                AUTO(BUILTIN_UNDEFINED)
                AUTO(BUILTIN_DOUBLE)
                AUTO(BUILTIN_INT8)
                AUTO(BUILTIN_INT16)
                AUTO(BUILTIN_INT32)
                AUTO(BUILTIN_INT64)
                AUTO(BUILTIN_GENERIC_BYTE)
                AUTO(BUILTIN_CHAR)
        }
#undef AUTO
}

void astdtype_free(struct astdtype *adt)
{
        switch (adt->type) {
                case ASTDTYPE_POINTER:
                        // DON'T!
//                        astdtype_free(adt->pointer.to);
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

struct astdtype *astdtype_void()
{
        return astdtype_generic(ASTDTYPE_VOID);
}

struct astdtype *astdtype_custom(char *name)
{
        struct astdtype *wrapper = astdtype_generic(ASTDTYPE_CUSTOM);
        wrapper->custom.name = strdup(name);
        return wrapper;
}

#define MAX_TYPENAME_LENGTH 200

char typename[200] = {0};

static void astdtype_append_typename(struct astdtype *type)
{
        if (type->type == ASTDTYPE_VOID) {
                strcat(typename, "void");
                return;
        }

        if (type->type == ASTDTYPE_CUSTOM) {
                strcat(typename, type->custom.name);
                return;
        }

        if (type->type == ASTDTYPE_BUILTIN) {
                switch (type->builtin.datatype) {
                        case BUILTIN_DOUBLE:
                                strcat(typename, "double");
                                break;
                        case BUILTIN_INT8:
                                strcat(typename, "int8");
                                break;
                        case BUILTIN_INT16:
                                strcat(typename, "int16");
                                break;
                        case BUILTIN_INT32:
                                strcat(typename, "int32");
                                break;
                        case BUILTIN_INT64:
                                strcat(typename, "int64");
                                break;
                        case BUILTIN_CHAR:
                                strcat(typename, "char");
                                break;
                        case BUILTIN_GENERIC_BYTE:
                                strcat(typename, "byte");
                                break;
                        default:
                                strcat(typename, "<Unknown>");
                                break;
                }
                return;
        }

        if (type->type == ASTDTYPE_POINTER) {
                strcat(typename, "ptr(");
                astdtype_append_typename(type->pointer.to);
                strcat(typename, ")");
                return;
        }
}

char *astdtype_string(struct astdtype *type)
{
        memset(typename, 0, MAX_TYPENAME_LENGTH - 1);
        astdtype_append_typename(type);
        typename[MAX_TYPENAME_LENGTH - 1] = '\n';
        return typename;
}

#undef MAX_TYPENAME_LENGTH


const char *nodetype_string(enum nodetype type)
{
        switch (type) {
#define AUTO(type) case type: return #type;
                AUTO(NODE_BLOCK)
                AUTO(NODE_PROGRAM)
                AUTO(NODE_FLOAT_LITERAL)
                AUTO(NODE_INTEGER_LITERAL)
                AUTO(NODE_STRING_LITERAL)
                AUTO(NODE_BINARY_OP)
                AUTO(NODE_VARIABLE_DECL)
                AUTO(NODE_POINTER)
                AUTO(NODE_VARIABLE_USE)
                AUTO(NODE_VARIABLE_ASSIGNMENT)
                AUTO(NODE_FUNCTION_DEFINITION)
                AUTO(NODE_FUNCTION_CALL)
                AUTO(NODE_SYMBOL)
                AUTO(NODE_RESOLVE)
                AUTO(NODE_DATA_TYPE)
#undef AUTO
                default:
                        return "Unknown Node";
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

char *symbol_type_humanstr(enum symbol_type type)
{
        switch (type) {
                case SYMBOL_FUNCTION:
                        return "function";
                case SYMBOL_VARIABLE:
                        return "variable";
                case SYMBOL_TYPEDEF:
                        return "type";
                default:
                        return "symbol of unknown type";
        }
}

void astnode_free(struct astnode *node)
{
        if (!node)
                return;

        switch (node->type) {
                case NODE_PROGRAM:
                        astnode_free(node->program.block);
                        break;
                case NODE_COMPOUND:
                        astnode_free_compound(node);
                        return;
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
                case NODE_VARIABLE_DECL:
                        astnode_free(node->declaration.value);
                        free(node->declaration.identifier);
//                        astdtype_free(node->declaration.type);
                        break;
                case NODE_POINTER:
                        astnode_free(node->pointer.target);
                        break;
                case NODE_VARIABLE_USE:
                        free(node->variable.identifier);
                        break;
                case NODE_VARIABLE_ASSIGNMENT:
                        free(node->assignment.identifier);
                        astnode_free(node->assignment.value);
                        break;
                case NODE_FUNCTION_DEFINITION:
                        free(node->function_def.identifier);
                        astnode_free(node->function_def.params);
                        astnode_free(node->function_def.block);
                        astnode_free(node->function_def.capture);
//                        astdtype_free(node->function_def.type);
                        break;
                case NODE_FUNCTION_CALL:
                        free(node->function_call.identifier);
                        astnode_free(node->function_call.values);
                        break;
                case NODE_RESOLVE:
                        astnode_free(node->resolve.value);
                        break;
                case NODE_DATA_TYPE:
                        astdtype_free(node->data_type.adt);
                        break;
                default:
                        break;
        }

        free(node);
}

struct astnode *astnode_generic(enum nodetype type, size_t line, struct astnode *block)
{
        struct astnode *node = malloc(sizeof(struct astnode));
        node->type = type;
        node->line = line;
        node->super = block;
        node->holder = NULL;
        return node;
}

struct astnode *astnode_nothing(size_t line, struct astnode *block)
{
        return astnode_generic(NODE_NOTHING, line, block);
}

struct astnode *astnode_empty_program()
{
        struct astnode *node = astnode_generic(NODE_PROGRAM, 0, NULL);
        node->program.block = NULL;
        return node;
}

struct astnode *astnode_empty_compound(size_t line, struct astnode *block)
{
        struct astnode *node = astnode_generic(NODE_COMPOUND, line, block);

        node->node_compound.max_count = NODE_ARRAY_INCREMENT;
        node->node_compound.count = 0;
        node->node_compound.array = calloc(node->node_compound.max_count, sizeof(struct astnode *));

        return node;
}

struct astnode *astnode_empty_block(size_t line, struct astnode *block)
{
        struct astnode *node = astnode_generic(NODE_BLOCK, line, block);
        node->block.nodes = astnode_empty_compound(line, node);
        node->block.symbols = astnode_empty_compound(line, node);
        return node;
}

_Bool astnode_push_compound(struct astnode *compound, struct astnode *node)
{
        _Bool resize = false;

        // Resize the array if needed
        if (compound->node_compound.count >= compound->node_compound.max_count) {
                compound->node_compound.array = realloc(compound->node_compound.array, sizeof(struct astnode *) *
                                                                                       (compound->node_compound.max_count += NODE_ARRAY_INCREMENT));
                resize = true;
        }

        compound->node_compound.array[compound->node_compound.count++] = node;
        return resize;
}

void *astnode_compound_foreach(struct astnode *compound, void *param, void *(*fun)(void *, struct astnode *))
{
        void *result;

        for (size_t i = 0; i < compound->node_compound.count; i++) {
                result = fun(param, compound->node_compound.array[i]);
                if (result)
                        return result;
        }

        return NULL;
}

void astnode_free_compound(struct astnode *compound)
{
        for (size_t i = 0; i < compound->node_compound.count; i++) {
                astnode_free(compound->node_compound.array[i]);
        }

        free(compound->node_compound.array);
        free(compound);
}

void astnode_free_block(struct astnode *block)
{
        astnode_free_compound(block->block.nodes);
        astnode_free_compound(block->block.symbols);
        free(block);
}

struct astnode *astnode_float_literal(size_t line, struct astnode *block, double value)
{
        struct astnode *node = astnode_generic(NODE_FLOAT_LITERAL, line, block);
        node->float_literal.floatValue = value;
        return node;
}

struct astnode *astnode_integer_literal(size_t line, struct astnode *block, int64_t value)
{
        struct astnode *node = astnode_generic(NODE_INTEGER_LITERAL, line, block);
        node->integer_literal.integerValue = value;
        return node;
}

struct astnode *astnode_string_literal(size_t line, struct astnode *block, char *value)
{
        struct astnode *node = astnode_generic(NODE_STRING_LITERAL, line, block);
        node->string_literal.value = strdup(value);
        node->string_literal.length = strlen(value);
        return node;
}

struct astnode *astnode_binary(size_t line, struct astnode *block, struct astnode *left, struct astnode *right, enum binaryop op)
{
        struct astnode *node = astnode_generic(NODE_BINARY_OP, line, block);
        node->binary.left = left;
        node->binary.right = right;
        node->binary.op = op;
        return node;
}

struct astnode *astnode_declaration(size_t line, struct astnode *block, _Bool constant, char *str, struct astdtype *type, struct astnode *value)
{
        struct astnode *node = astnode_generic(NODE_VARIABLE_DECL, line, block);
        node->declaration.constant = constant;
        node->declaration.type = type;
        node->declaration.value = value;
        node->declaration.identifier = strdup(str);
        return node;
}

struct astnode *astnode_pointer(size_t line, struct astnode *block, struct astnode *to)
{
        struct astnode *node = astnode_generic(NODE_POINTER, line, block);
        node->pointer.target = to;
        return node;
}

struct astnode *astnode_variable(size_t line, struct astnode *block, char *str)
{
        struct astnode *node = astnode_generic(NODE_VARIABLE_USE, line, block);
        node->variable.identifier = strdup(str);
        return node;
}

struct astnode *astnode_assignment(size_t line, struct astnode *block, char *identifier, struct astnode *value)
{
        struct astnode *node = astnode_generic(NODE_VARIABLE_ASSIGNMENT, line, block);
        node->assignment.value = value;
        node->assignment.identifier = strdup(identifier);
        return node;
}

struct astnode *astnode_function_definition(size_t line, struct astnode *superblock, char *identifier, struct astnode *parameters, struct astdtype *type, struct astnode *capture, struct astnode *block)
{
        struct astnode *node = astnode_generic(NODE_FUNCTION_DEFINITION, line, superblock);
        node->function_def.identifier = strdup(identifier);
        node->function_def.params = parameters;
        node->function_def.type = type;
        node->function_def.block = block;
        node->function_def.capture = capture;
        return node;
}

struct astnode *astnode_function_call(size_t line, struct astnode *block, char *identifier, struct astnode *values)
{
        struct astnode *node = astnode_generic(NODE_FUNCTION_CALL, line, block);
        node->function_call.identifier = strdup(identifier);
        node->function_call.values = values;
        return node;
}

struct astnode *astnode_resolve(size_t line, struct astnode *block, struct astnode *value)
{
        struct astnode *node = astnode_generic(NODE_RESOLVE, line, block);
        node->resolve.value = value;
        node->resolve.function = NULL;
        return node;
}

struct astnode *astnode_data_type(struct astdtype *type)
{
        struct astnode *node = astnode_generic(NODE_DATA_TYPE, 0, NULL);
        node->data_type.adt = type;
        return node;
}

struct astnode *astnode_symbol(struct astnode *block, enum symbol_type t, char *id, struct astdtype *type, struct astnode *orig)
{
        struct astnode *node = astnode_generic(NODE_SYMBOL, block->line, block);
        node->symbol.symtype = t;
        node->symbol.identifier = id;
        node->symbol.type = type;
        node->symbol.node = orig;
        return node;
}