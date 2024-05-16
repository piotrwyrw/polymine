#ifndef SEMUTIL_H
#define SEMUTIL_H

#include "../common/ast.h"
#include "semantics.h"

struct semantics {
        struct astdtype *int8;
        struct astdtype *int16;
        struct astdtype *int32;
        struct astdtype *int64;
        struct astdtype *_double;
        struct astdtype *byte;
        struct astdtype *_void;
        struct astdtype *string;

        struct astnode *program;

        struct astnode *stuff;

        size_t symbol_counter;

        _Bool pristine; // TRUE if nothing but include nodes were analyzed up until this time
};

void semantics_init(struct semantics *, struct astnode *types, struct astnode *program);

void semantics_free(struct semantics *);

enum traverse_params {
        TRAVERSE_SYMBOLS = (1 << 0),
        TRAVERSE_NODES = (1 << 1),

        // The upward traversal should not proceed after the block belonging to a nested function is analysed.
        TRAVERSE_HALT_NESTED = (1 << 2),

        // The global scope/block should be traversed, even if the search is halted by TRAVERSE_HALT_NESTED
        TRAVERSE_GLOBAL_REGARDLESS = (1 << 3)
};

struct astnode *custom_traverse(void *, void *(*)(void *, struct astnode *), struct astnode *, enum traverse_params);

struct astnode *find_symbol(char *, struct astnode *);

struct astnode *find_symbol_shallow(char *, struct astnode *);

struct astnode *find_enclosing_function(struct astnode *);

struct astnode *find_uncertain_reachability_structures(struct astnode *);

struct astnode *find_in_type(struct astnode *complex, char *id);

void put_symbol(struct astnode *, struct astnode *);

_Bool is_uppermost_block(struct astnode *);

struct astnode *last_call_path(struct astnode *, struct astnode *);

_Bool types_compatible(struct astdtype *, struct astdtype *);

size_t quantify_type_size(struct astdtype *);

struct astdtype *required_type(struct astdtype *, struct astdtype *);

struct astdtype *required_type_integer(struct semantics *sem, int64_t value);

struct astdtype *semantics_new_type(struct semantics *, struct astdtype *);

size_t find_pointer_degree(struct astdtype *, struct astdtype **);

void semantics_new_function(struct semantics *, struct astnode *);

void semantics_new_include(struct semantics *, char *);

_Bool has_attribute(struct astnode *, char const *);

/**
 * Note; This will only check for CONFLICTS, thus only in the current block, since
 * variable shadowing is supported
 */
struct astnode *symbol_conflict(char *, struct astnode *);

#endif
