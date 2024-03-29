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

struct astnode *find_symbol(char *, struct astnode *);

void put_symbol(struct astnode *, struct astnode *);

_Bool compare_types(struct astdtype *, struct astdtype *);

size_t quantify_type_size(struct astdtype *);

struct astdtype *required_type(struct astdtype *, struct astdtype *);

struct astdtype *required_type_integer(struct semantics *, int);

#endif
