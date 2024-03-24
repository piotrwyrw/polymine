#include "lexer.h"
#include "defs.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

const char *lxtype_string(enum lxtype type)
{
#define AUTO_CASE(type) case type: return #type;
        switch (type) {
                AUTO_CASE(LX_UNDEFINED)
                AUTO_CASE(LX_IDEN)
                AUTO_CASE(LX_INTEGER)
                AUTO_CASE(LX_DECIMAL)
                AUTO_CASE(LX_STRING)
                AUTO_CASE(LX_DOUBLE_EQUALS)
                AUTO_CASE(LX_LGREATER)
                AUTO_CASE(LX_RGREATER)
                AUTO_CASE(LX_LGREQUAL)
                AUTO_CASE(LX_RGREQUAL)
                AUTO_CASE(LX_INCREMENT)
                AUTO_CASE(LX_DECREMENT)
                AUTO_CASE(LX_SHL)
                AUTO_CASE(LX_SHR)
                AUTO_CASE(LX_MOV_LEFT)
                AUTO_CASE(LX_MOV_RIGHT)
                AUTO_CASE(LX_EQUALS)
                AUTO_CASE(LX_LSQUARE)
                AUTO_CASE(LX_RSQUARE)
                AUTO_CASE(LX_LPAREN)
                AUTO_CASE(LX_RPAREN)
                AUTO_CASE(LX_LBRACE)
                AUTO_CASE(LX_RBRACE)
                AUTO_CASE(LX_SEMI)
                AUTO_CASE(LX_COLON)
                AUTO_CASE(LX_MINUS)
                AUTO_CASE(LX_PLUS)
                AUTO_CASE(LX_SLASH)
                AUTO_CASE(LX_ASTERISK)
                AUTO_CASE(LX_AMPERSAND)
                AUTO_CASE(LX_POW)
                AUTO_CASE(LX_MOD)
                AUTO_CASE(LX_DOLLAR)
                AUTO_CASE(LX_HASH)
                AUTO_CASE(LX_AT)
                AUTO_CASE(LX_EXCLAMATION)
                AUTO_CASE(LX_DOT)
                AUTO_CASE(LX_COMMA)
                default:
                        return "[??]";
        }
#undef AUTO_CASE
}

void lexer_init(struct lexer *obj, struct input_handle const *handle)
{
        obj->input = handle;
        obj->position = 0;
        obj->line = 1;
}

void lxtok_init(struct lxtok *tok, enum lxtype type, char *val, size_t line)
{
        tok->type = type;
        if (val)
                tok->value = strdup(val);
        else
                tok->value = NULL;
        tok->line = line;
}

void lxtok_free(struct lxtok *tok)
{
        if (tok->value) {
                free(tok->value);
                tok->value = NULL;
        }

        tok->line = 0;
        tok->type = LX_UNDEFINED;
}

_Bool lexer_skip_spaces(struct lexer *lx)
{
        if (lx->position >= lx->input->length)
                return false;

        for (; lx->position < lx->input->length; lx->position++) {
                char c = lx->input->buffer[lx->position];
                if (isspace(c)) {
                        if (c == '\n')
                                lx->line++;
                        continue;
                }

                return true;
        }

        return false;
}

inline _Bool lexer_empty(struct lexer *lx)
{
        return !lexer_skip_spaces(lx);
}

_Bool lexer_next(struct lexer *lx, struct lxtok *tok)
{
        enum lxmode mode = LMODE_NOTHING;
        enum lxtype type = LX_UNDEFINED;
        size_t line = lx->line;

        char *buffer = calloc(max_token_length, sizeof(char));
        size_t buffer_idx = 0;

        for (; lx->position < lx->input->length; lx->position++) {
                char c = lx->input->buffer[lx->position];

                // If this is the first character of the new token, and thus the mode
                // is still unknown, try to determine it here
                if (mode == LMODE_NOTHING) {
                        if (is_iden_char(c)) {
                                mode = LMODE_IDENTIFIER;
                                type = LX_IDEN;
                        } else if (is_number_char(c)) {
                                mode = LMODE_NUMBER;
                                type = LX_INTEGER;
                        }
                }

                // The mode selection for string literals works in a different way
                if (c == '"') {
                        if (mode == LMODE_STRING) {
                                lx->position++; // Skip past the '"' so it does not come up in the following run
                                goto submit_token;
                        }
                        mode = LMODE_STRING;
                        type = LX_STRING;
                        continue; // The '"' character does not constitute a part of the string
                }

                // The tokenizer should be in some known state by now. If it's not the case,
                // try to parse a special construct
                if (mode == LMODE_NOTHING) {
                        if (special_constr(lx, tok)) {
                                goto free_and_exit;
                        }

                        // If we don't know what the current token is, skip it, and return the next one.
                        DEBUG({
                                      printf("Unknown token starting with \"%c\" on line %ld.\n",
                                             lx->input->buffer[lx->position], lx->line);
                              });

                        lx->position++;
                        goto reset_start_over;
                }

                if (mode == LMODE_IDENTIFIER) {
                        if (!is_iden_char(c) && (!is_number_char(c) && buffer_idx > 0)) {
                                goto submit_token;
                        }
                }

                if (mode == LMODE_NUMBER || mode == LMODE_DECIMAL) {
                        if (!is_number_char(c)) {
                                if (c != '.' || mode != LMODE_NUMBER) {
                                        lx->position --;
                                        goto submit_token;
                                }

                                mode = LMODE_DECIMAL;
                                type = LX_DECIMAL;
                        }
                }

                if (mode == LMODE_STRING && c == '\\') {
                        if (!escape_sequence(lx, buffer, &buffer_idx)) {
                                printf("Bad escape sequence on line %ld.", lx->line);
                                return false;
                        }

                        lx->position--;

                        continue;
                }

                buffer[buffer_idx++] = c;

                if (lx->position + 1 >= lx->input->length)
                        goto submit_token;

                continue;

                reset_start_over:
                mode = LMODE_NOTHING;
                type = LX_UNDEFINED;
                memset(buffer, 0, max_token_length);
                buffer_idx = 0;

                if (!lexer_skip_spaces(lx))
                        goto free_and_exit;

                line = lx->line;

                // Continuing the loop will result in lx->position being incremented. The skip spaces method above skipped
                // all the spaces, thus the position is currently pointing to the first non-space character of the next token.
                // Therefore, we need to subtract 1 from said pointer to avoid cutting of (part) of the token
                lx->position--;

        } // End of for loop

        return true;

        submit_token:
        lxtok_init(tok, type, buffer, line);
        lx->position++;

        free_and_exit:
        free(buffer);
        return true;
}

_Bool special_constr(struct lexer *lx, struct lxtok *tok)
{
        if (lx->position >= lx->input->length)
                return false;

        size_t line = lx->line;
        char c1 = lx->input->buffer[lx->position], c2;
        enum lxtype type = LX_UNDEFINED;

        if (lx->position + 1 < lx->input->length) {
                c2 = lx->input->buffer[lx->position + 1];
                if (isspace(c2) || is_iden_char(c2) || is_number_char(c2))
                        c2 = '\0';
        } else
                c2 = '\0';

        char str[] = {c1, c2, 0};

#define SUBMIT_TYPE_ON(cmpstr, t)                \
        if (strcmp(str, cmpstr) == 0) {                \
                type = t;                        \
                goto submit;                        \
        }

        SUBMIT_TYPE_ON("=", LX_EQUALS);
        SUBMIT_TYPE_ON("==", LX_DOUBLE_EQUALS);
        SUBMIT_TYPE_ON(">", LX_LGREATER);
        SUBMIT_TYPE_ON("<", LX_RGREATER);
        SUBMIT_TYPE_ON(">=", LX_LGREQUAL);
        SUBMIT_TYPE_ON("<=", LX_RGREQUAL);
        SUBMIT_TYPE_ON("++", LX_INCREMENT);
        SUBMIT_TYPE_ON("--", LX_DECREMENT);
        SUBMIT_TYPE_ON("<<", LX_SHL);
        SUBMIT_TYPE_ON(">>", LX_SHR);
        SUBMIT_TYPE_ON("<-", LX_MOV_LEFT);
        SUBMIT_TYPE_ON("->", LX_MOV_RIGHT);

#undef SUBMIT_TYPE_ON
#define SUBMIT_TYPE_ON(cmpc, t)                        \
        if (c1 == cmpc) {                        \
                type = t;                        \
                goto submit;                        \
        }

        // The special construct is not a compound. We know that by now.
        // Thus, we shall erase the second character from the string to
        // aboid returning a second character from now on. Also, erase c2 to
        // avoid incrementing the position pointer by 2.
        str[1] = 0;
        c2 = 0;

        SUBMIT_TYPE_ON('[', LX_LSQUARE);
        SUBMIT_TYPE_ON(']', LX_RSQUARE);
        SUBMIT_TYPE_ON('(', LX_LPAREN);
        SUBMIT_TYPE_ON(')', LX_RPAREN);
        SUBMIT_TYPE_ON('{', LX_LBRACE);
        SUBMIT_TYPE_ON('}', LX_RBRACE);
        SUBMIT_TYPE_ON(';', LX_SEMI);
        SUBMIT_TYPE_ON(':', LX_COLON);
        SUBMIT_TYPE_ON('-', LX_MINUS);
        SUBMIT_TYPE_ON('+', LX_PLUS);
        SUBMIT_TYPE_ON('/', LX_SLASH);
        SUBMIT_TYPE_ON('*', LX_ASTERISK);
        SUBMIT_TYPE_ON('&', LX_AMPERSAND);
        SUBMIT_TYPE_ON('^', LX_POW);
        SUBMIT_TYPE_ON('%', LX_MOD);
        SUBMIT_TYPE_ON('$', LX_DOLLAR);
        SUBMIT_TYPE_ON('#', LX_HASH);
        SUBMIT_TYPE_ON('@', LX_AT);
        SUBMIT_TYPE_ON('!', LX_EXCLAMATION);
        SUBMIT_TYPE_ON('.', LX_DOT);
        SUBMIT_TYPE_ON(',', LX_COMMA);

#undef SUBMIT_TYPE_ON

        return false;

        submit:

        if (c2)
                lx->position += 2;
        else
                lx->position++;

        lxtok_init(tok, type, str, line);
        return true;
}

_Bool escape_sequence(struct lexer *lx, char *buffer, size_t *buffIdx)
{
        if (lx->position + 1 >= lx->input->length)
                return false;

        char junk = lx->input->buffer[lx->position];
        if (junk != '\\')
                return false;

        char seq = lx->input->buffer[lx->position + 1];

        char subst;

        switch (seq) {
                case 'n':
                        subst = '\n';
                        break;
                case '\\':
                        subst = '\\';
                        break;
                case '\"':
                        subst = '\"';
                        break;
                defaul:
                        return false;
        }

        lx->position += 2; // Skip the two-character-long escape sequence
        buffer[*buffIdx] = subst;
        *buffIdx = (*buffIdx) + 1;

        return true;
}

inline _Bool is_iden_char(char c)
{
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

inline _Bool is_number_char(char c)
{
        return (c >= '0' && c <= '9');
}
