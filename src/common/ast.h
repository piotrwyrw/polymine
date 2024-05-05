#ifndef AST_H
#define AST_H

#include "../syntax/lexer.h"

#include <stdlib.h>
#include <stdbool.h>

#define NODE_ARRAY_INCREMENT 20

extern _Bool freeDataTypes;

enum nodetype : uint8_t {
        NODE_UNDEFINED = 0,

        NODE_NOTHING, // Warning: This is NOT the same as UNDEFINED !! Something along the lines of a NOP
        NODE_COMPOUND,
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
        NODE_ATTRIBUTE,
        NODE_RESOLVE,
        NODE_DATA_TYPE,
        NODE_VOID_PLACEHOLDER,  // Used in place of 'NULL' e.g. for resolve statements with the return type void
        NODE_IF,

        // Semantic stuff
        NODE_SYMBOL,
        NODE_GENERATED_FUNCTION,
        NODE_INCLUDE,

        // Memory safety
        NODE_WRAPPED
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

enum symbol_type {
        SYMBOL_VARIABLE,
        SYMBOL_FUNCTION,
        SYMBOL_TYPEDEF
};

char *symbol_type_humanstr(enum symbol_type);

enum gen_type {
        GENERATED_REGULAR,
        GENERATED_LAMBDA
};

struct astdtype;

struct astnode {
        enum nodetype type;
        size_t line;
        struct astnode *super; // The enclosing block
        struct astnode *holder; // The holder node. NULL most of the time.
        union {
                struct {
                        struct astnode *block;
                } program;

                struct {
                        struct astnode *nodes;
                        struct astnode *symbols;
                } block;

                struct {
                        struct astnode **array;
                        size_t max_count;
                        size_t count;
                } node_compound;

                struct {
                        double floatValue;
                } float_literal;

                struct {
                        int64_t integerValue;
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
                        struct astnode *capture; // And so is this!
                        struct astnode *block;
                        _Bool nested;
                        _Bool conditionless_resolve; // Managed by Semantic Analysis
                        struct astnode *attributes; // Compound
                        struct astnode *generated; // generated_function
                } function_def;

                struct {
                        char *identifier;
                        struct astnode *values; // And so is this!
                } function_call;

                struct {
                        char *identifier;
                } attribute;

                struct {
                        struct astnode *value;
                        struct astnode *function; // Managed by the semantic analysis
                } resolve;

                struct {
                        struct astnode *expr;
                        struct astnode *block;
                        struct astnode *next_branch;
                } if_statement;

                // -- Stuff for semantic analysis --
                struct {
                        enum symbol_type symtype;
                        char *identifier;
                        struct astdtype *type;
                        struct astnode *node;
                } symbol;

                struct {
                        struct astnode *definition;
                        size_t number;
                        enum gen_type type;
                        char *generated_id;
                } generated_function;

                struct {
                        char *path;
                } include;

                // -- Memory safety features --
                struct {
                        struct astnode *node;
                } wrapped_node;
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
        ASTDTYPE_LAMBDA
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

                struct {
                        struct astnode *paramTypes;
                        struct astdtype *returnType;
                } lambda;
        };
};

void astdtype_free(struct astdtype *);

struct astdtype *astdtype_generic(enum astdtype_type);

struct astdtype *astdtype_pointer(struct astdtype *);

struct astdtype *astdtype_builtin(enum builtin_type);

struct astdtype *astdtype_void();

struct astdtype *astdtype_custom(char *);

struct astdtype *astdtype_lambda(struct astnode *, struct astdtype *);

char *astdtype_string(struct astdtype *);

/* Free a node recursively */
void astnode_free(struct astnode *);

struct astnode *astnode_generic(enum nodetype, size_t, struct astnode *);

struct astnode *astnode_nothing(size_t, struct astnode *);

struct astnode *astnode_empty_program();

struct astnode *astnode_empty_compound(size_t, struct astnode *);

_Bool astnode_push_compound(struct astnode *, struct astnode *);

void *astnode_compound_foreach(struct astnode *, void *, void *(*)(void *, struct astnode *));

void astnode_free_compound(struct astnode *);

void astnode_free_block(struct astnode *);

struct astnode *astnode_empty_block(size_t, struct astnode *);

struct astnode *astnode_float_literal(size_t, struct astnode *, double);

struct astnode *astnode_integer_literal(size_t, struct astnode *, int64_t);

struct astnode *astnode_string_literal(size_t, struct astnode *, char *);

struct astnode *astnode_binary(size_t, struct astnode *, struct astnode *, struct astnode *, enum binaryop);

struct astnode *astnode_declaration(size_t, struct astnode *, _Bool, char *, struct astdtype *, struct astnode *);

struct astnode *astnode_pointer(size_t, struct astnode *, struct astnode *);

struct astnode *astnode_variable(size_t, struct astnode *, char *);

struct astnode *astnode_assignment(size_t, struct astnode *, char *, struct astnode *);

struct astnode *astnode_function_definition(size_t, struct astnode *, char *, struct astnode *, struct astdtype *, struct astnode *, struct astnode *, struct astnode *);

struct astnode *astnode_function_call(size_t, struct astnode *, char *, struct astnode *);

struct astnode *astnode_attribute(size_t, struct astnode *, char *);

struct astnode *astnode_resolve(size_t, struct astnode *, struct astnode *);

struct astnode *astnode_data_type(struct astdtype *);

struct astnode *astnode_void_placeholder(size_t, struct astnode *);

struct astnode *astnode_if(size_t, struct astnode *, struct astnode *, struct astnode *, struct astnode *);

// Semantic nodes --

struct astnode *astnode_symbol(struct astnode *, enum symbol_type, char *, struct astdtype *, struct astnode *);

struct astnode *astnode_copy_symbol(struct astnode *);

struct astnode *astnode_generated_function(struct astnode *, size_t, enum gen_type);

struct astnode *astnode_include(size_t, struct astnode *super, char *);

// Memory safety --

struct astnode *astnode_wrap(struct astnode *);

struct astnode *astnode_unwrap(struct astnode *);

#ifndef WRAP
#define WRAP(n) astnode_wrap(n)
#endif

#ifndef UNWRAP
#define UNWRAP(n) astnode_unwrap(n)
#endif

#endif