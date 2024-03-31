#ifndef SEMUTIL_H
#define SEMUTIL_H

#include "ast.h"
#include "semantics.h"

struct semantics {
        struct astdtype *int8;
        struct astdtype *int16;
        struct astdtype *int32;
        struct astdtype *int64;
        struct astdtype *_double;
        struct astdtype *byte;
};

void semantics_init(struct semantics *);

void semantics_free(struct semantics *);

enum traversal_domain {
        TRAVERSE_SYMBOLS,
        TRAVERSE_NODES
};

struct astnode *custom_traverse(void *, void *(*)(void *, struct astnode *), struct astnode *, enum traversal_domain);

struct astnode *find_symbol(char *, struct astnode *);

struct astnode *find_enclosing_function(struct astnode *);

void put_symbol(struct astnode *, struct astnode *);

_Bool types_compatible(struct astdtype *, struct astdtype *);

size_t quantify_type_size(struct astdtype *);

struct astdtype *required_type(struct astdtype *, struct astdtype *);

struct astdtype *required_type_integer(struct semantics *, int);

/**
 * Note; This will only check for CONFLICTS, thus only in the current block, since
 * variable shadowing is supported
 */
struct astnode *symbol_conflict(char *, struct astnode *);

#endif
