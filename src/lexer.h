#ifndef LEXER_H
#define LEXER_H

#include "io.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

const static size_t max_token_length = 100;

enum lxtype : uint8_t {
        LX_UNDEFINED = 0,

        LX_IDEN,
        LX_INTEGER,
        LX_DECIMAL,
        LX_STRING,

        // Compounds
        LX_DOUBLE_EQUALS,
        LX_LGREATER,
        LX_RGREATER,
        LX_LGREQUAL,
        LX_RGREQUAL,
        LX_INCREMENT,
        LX_DECREMENT,
        LX_SHL,
        LX_SHR,
        LX_MOV_LEFT,
        LX_MOV_RIGHT,
        LX_DOUBLE_OR,

        LX_EQUALS,
        LX_LSQUARE,
        LX_RSQUARE,
        LX_LPAREN,
        LX_RPAREN,
        LX_LBRACE,
        LX_RBRACE,
        LX_SEMI,
        LX_COLON,
        LX_MINUS,
        LX_PLUS,
        LX_SLASH,
        LX_ASTERISK,
        LX_QUESTION_MARK,
        LX_AMPERSAND,
        LX_POW,
        LX_MOD,
        LX_DOLLAR,
        LX_HASH,
        LX_AT,
        LX_EXCLAMATION,
        LX_DOT,
        LX_COMMA
};

const char *lxtype_string(enum lxtype);

struct lxtok {
        enum lxtype type;
        char *value;
        size_t line;
};

void lxtok_init(struct lxtok *, enum lxtype, char *, size_t);

void lxtok_free(struct lxtok *);

// This is used internally bt the tokenizer to keep track
// of what token category it is now "parsing"
enum lxmode : uint8_t {
        LMODE_NOTHING = 0,

        LMODE_IDENTIFIER,
        LMODE_NUMBER,
        LMODE_DECIMAL,
        LMODE_STRING,
        LMODE_SPECIAL
};

struct lexer {
        struct input_handle const *input;
        size_t position;
        size_t line;
};

void lexer_init(struct lexer *, struct input_handle const *);

_Bool lexer_skip_spaces(struct lexer *);

_Bool lexer_empty(struct lexer *);

_Bool lexer_next(struct lexer *, struct lxtok *);

_Bool special_constr(struct lexer *, struct lxtok *);

_Bool escape_sequence(struct lexer *, char *buffer, size_t *buffIdx);

_Bool is_iden_char(char);

_Bool is_number_char(char);

#endif

