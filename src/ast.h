#ifndef AST_H
#define AST_H

#include "lexer.h"

#include <stdlib.h>
#include <stdbool.h>

#define NODE_ARRAY_INCREMENT 20

enum nodetype : uint8_t {
        NODE_UNDEFINED = 0,

        NODE_NOTHING, // Warning: This is NOT the same as UNDEFINED !!
        NODE_BLOCK,
        NODE_PROGRAM,
        NODE_NUMBER_LITERAL,
        NODE_STRING_LITERAL,
        NODE_BINARY_OP
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
                        double value;
                } number_literal;

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
        };
};

enum builtin_type : uint8_t {
        BUILTIN_UNDEFINED = 0,
        BUILTIN_CHAR,
        BUILTIN_NUMBER,
        BUILTIN_GENERIC_BYTE
};

enum builtin_type builtin_from_string(char *);

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

struct astdtype *astdtype_custom(char *);

/* Recursively free a node */
void astnode_free(struct astnode *);

struct astnode *astnode_nothing(size_t);

struct astnode *astnode_empty_program();

struct astnode *astnode_empty_block(size_t);

_Bool astnode_push_block(struct astnode *, struct astnode *);

void astnode_free_block(struct astnode *);

struct astnode *astnode_generic(enum nodetype, size_t);

struct astnode *astnode_number_literal(size_t, double);

struct astnode *astnode_string_literal(size_t, char *);

struct astnode *astnode_binary(size_t, struct astnode *, struct astnode *, enum binaryop);

#endif
