#ifndef AST_H
#define AST_H

#include "lexer.h"

#include <stdlib.h>
#include <stdbool.h>

#define NODE_ARRAY_INCREMENT 20

enum nodetype : uint8_t {
        NODE_UNDEFINED = 0,

        NODE_NOTHING, // Warning: This is NOT the same as UNDEFINED !! Something along the lines on NOP
        NODE_BLOCK,
        NODE_PROGRAM,
        NODE_FLOAT_LITERAL,
        NODE_INTEGER_LITERAL,
        NODE_STRING_LITERAL,
        NODE_BINARY_OP,
        NODE_VARIABLE_DECL,
        NODE_POINTER,
        NODE_VARIABLE_USE,
        NODE_VARIABLE_ASSIGNMENT,
        NODE_FUNCTION_DEFINITION,
        NODE_FUNCTION_CALL,
        NODE_RESOLVE
};

const char *nodetype_string(enum nodetype);

enum binaryop : uint8_t {
        BOP_UNKNOWN = 0,

        BOP_ADD,
        BOP_SUB,
        BOP_MUL,
        BOP_DIV
};

enum binaryop bop_from_lxtype(enum lxtype);
const char *binaryop_string(enum binaryop);

struct astdtype;

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
                        double floatValue;
                } float_literal;

                struct {
                        int integerValue;
                } integer_literal;

                struct {
                        size_t length;
                        char *value;
                } string_literal;

                struct {
                        struct astdtype *adt;
                } data_type;

                struct {
                        struct astnode *left;
                        struct astnode *right;
                        enum binaryop op;
                } binary;

                struct {
                        _Bool constant;
                        char *identifier;
                        struct astdtype *type;
                        struct astnode *value;
                } declaration;

                struct {
                        struct astnode *target;
                } pointer;

                struct {
                        char *identifier;
                } variable;

                struct {
                        char *identifier;
                        struct astnode *value;
                } assignment;

                struct {
                        char *identifier;
                        struct astnode *params; // This is a block too!
                        struct astdtype *type;
                        struct astnode *block;
                } function_def;

                struct {
                        char *identifier;
                        struct astnode *values; // And so is this!
                } function_call;

                struct {
                        struct astnode *value;
                } resolve;
        };
};

enum builtin_type : uint8_t {
        BUILTIN_UNDEFINED = 0,
        BUILTIN_CHAR,
        BUILTIN_DOUBLE,
        BUILTIN_INT8,
        BUILTIN_INT16,
        BUILTIN_INT32,
        BUILTIN_INT64,
        BUILTIN_GENERIC_BYTE
};

enum builtin_type builtin_from_string(char *);

const char *builtin_string(enum builtin_type);

enum astdtype_type : uint8_t {
        ASTDTYPE_VOID = 0,
        ASTDTYPE_POINTER,
        ASTDTYPE_BUILTIN,
        ASTDTYPE_CUSTOM,
};

/**
 * Used to represent (complex) types
 */
struct astdtype {
        enum astdtype_type type;

        union {
                struct {
                        struct astdtype *to;
                } pointer;

                struct {
                        enum builtin_type datatype;
                } builtin;

                struct {
                        char *name;
                } custom;
        };
};

void astdtype_free(struct astdtype *);

struct astdtype *astdtype_generic(enum astdtype_type);

struct astdtype *astdtype_pointer(struct astdtype *);

struct astdtype *astdtype_builtin(enum builtin_type);

struct astdtype *astdtype_void();

struct astdtype *astdtype_custom(char *);

/* Recursively free a node */
void astnode_free(struct astnode *);

struct astnode *astnode_nothing(size_t);

struct astnode *astnode_empty_program();

struct astnode *astnode_empty_block(size_t);

_Bool astnode_push_block(struct astnode *, struct astnode *);

void astnode_free_block(struct astnode *);

struct astnode *astnode_generic(enum nodetype, size_t);

struct astnode *astnode_float_literal(size_t, double);

struct astnode *astnode_integer_literal(size_t, int);

struct astnode *astnode_string_literal(size_t, char *);

struct astnode *astnode_binary(size_t, struct astnode *, struct astnode *, enum binaryop);

struct astnode *astnode_declaration(size_t, _Bool, char *, struct astdtype *, struct astnode *);

struct astnode *astnode_pointer(size_t, struct astnode *);

struct astnode *astnode_variable(size_t, char *);

struct astnode *astnode_assignment(size_t, char *, struct astnode *);

struct astnode *astnode_function_definition(size_t, char *, struct astnode *, struct astdtype *, struct astnode *);

struct astnode *astnode_function_call(size_t, char *, struct astnode *);

struct astnode *astnode_resolve(size_t, struct astnode *);

#endif
