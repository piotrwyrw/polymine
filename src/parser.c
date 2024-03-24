#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "util.h"

#include <string.h>

void parser_init(struct parser *p, struct lexer *lx)
{
        p->lx = lx;
        p->line = 1;

        lxtok_init(&p->current, LX_UNDEFINED, NULL, 1);
        lxtok_init(&p->next, LX_UNDEFINED, NULL, 1);

        parser_advance(p);
        parser_advance(p);
}

void parser_free(struct parser *p)
{
        lxtok_free(&p->current);
        lxtok_free(&p->next);
}

void parser_advance(struct parser *p)
{
        lxtok_free(&p->current);
        p->current = p->next;

        if (p->current.line > p->line)
                p->line = p->current.line;

        if (lexer_empty(p->lx)) {
                lxtok_init(&p->next, LX_UNDEFINED, NULL, 0);
                return;
        }

        lexer_next(p->lx, &p->next);
}

static struct astnode *parser_parse_whatever(struct parser *p, enum pstatus *status)
{
        *status = PARSER_SYNTAX_ERROR;

        if (p->current.type == LX_UNDEFINED)
                goto exit_no_match;

        if (p->current.type == LX_STRING)
                return parse_string_literal(p);

        if (p->current.type == LX_IDEN) {
                if (strcmp(p->current.value, "nothing") == 0)
                        return parse_nothing(p);
        }

        if (p->current.type == LX_LBRACE)
                return parse_block(p);

        return parse_expr(p);

        exit_no_match:
        *status = PARSER_NO_MATCH;
        return NULL;
}

struct astnode *parser_parse(struct parser *p)
{
        struct astnode *program = astnode_empty_program();

        program->program.block = parse_block_advanced(p, false);

        if (!program->program.block)
                return NULL;

        return program;
}

struct astnode *parse_block_advanced(struct parser *p, _Bool decorated)
{
        if (decorated) {
                if (p->current.type != LX_LBRACE) {
                        printf("Expected '{' at the beginning of a block. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        return NULL;
                }

                parser_advance(p);
        }

        struct astnode *block = astnode_empty_block(p->line);
        struct astnode *node;
        enum pstatus status;

        // Keep parsing for as long as new tokens are available
        while (p->current.type != LX_UNDEFINED) {
                if (decorated && p->current.type == LX_RBRACE)
                        break;

                node = parser_parse_whatever(p, &status);

                if (!node) {
                        if (status != PARSER_SYNTAX_ERROR)
                                printf("Could not parse element starting with %s (\"%s\").\n",
                                       lxtype_string(p->current.type), p->current.value);
                        goto syntax_error;
                }

                astnode_push_block(block, node);
        }

        if (decorated) {
                if (p->current.type != LX_RBRACE) {
                        printf("Expected '}' at the end of a block statement. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        return NULL;
                }

                parser_advance(p);
        }

        return block;

        syntax_error:
        astnode_free(block);

        return NULL;
}

inline struct astnode *parse_block(struct parser *p)
{
        return parse_block_advanced(p, true);
}

struct astnode *parse_nothing(struct parser *p)
{
        if (p->current.type != LX_IDEN) {
                printf("Expected a nothing-identifier. Got %s (\"%s\") on line %ld.\n", lxtype_string(p->current.type),
                       p->current.value, p->line);
                return NULL;
        }

        size_t line = p->line;

        parser_advance(p);

        return astnode_nothing(line);
}

struct astdtype *parse_type(struct parser *p)
{
        char *identifier;

        if (p->current.type != LX_IDEN) {
                printf("Expected type name or functional identifier. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        identifier = p->current.value;

        enum builtin_type bt = builtin_from_string(identifier);

        if (bt != BUILTIN_UNDEFINED) {
                parser_advance(p);
                return astdtype_builtin(bt);
        }

        if (strcmp(identifier, "ptr") == 0) {
                parser_advance(p);

                if (p->current.type != LX_LPAREN) {
                        printf("Expected '(' after functional identifier. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        return NULL;
                }

                parser_advance(p);

                struct astdtype *enclosed = parse_type(p);

                if (!enclosed)
                        return NULL;

                if (p->current.type != LX_RPAREN) {
                        printf("Expected ')' after enclosed type. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        astdtype_free(enclosed);
                        return NULL;
                }

                parser_advance(p);

                return astdtype_pointer(enclosed);
        }

        struct astdtype *type = astdtype_custom(identifier);
        parser_advance(p);
        return type;
}

inline struct astnode *parse_expr(struct parser *p)
{
        return parse_additive_expr(p);
}

struct astnode *parse_additive_expr(struct parser *p)
{
        size_t line = p->line;
        struct astnode *left = parse_multiplicative_expr(p);

        if (!left)
                return NULL;

        while (p->current.type == LX_PLUS || p->current.type == LX_MINUS) {
                enum binaryop op = bop_from_lxtype(p->current.type);

                parser_advance(p);

                struct astnode *right = parse_additive_expr(p);

                if (!right) {
                        astnode_free(left);
                        return NULL;
                }

                left = astnode_binary(line, left, right, op);
        }

        return left;
}

struct astnode *parse_multiplicative_expr(struct parser *p)
{
        size_t line = p->line;
        struct astnode *left = parse_atom(p);

        if (!left)
                return NULL;

        while (p->current.type == LX_ASTERISK || p->current.type == LX_SLASH) {
                enum binaryop op = bop_from_lxtype(p->current.type);

                parser_advance(p);

                struct astnode *right = parse_multiplicative_expr(p);

                if (!right) {
                        astnode_free(left);
                        return NULL;
                }

                left = astnode_binary(line, left, right, op);
        }

        return left;
}

struct astnode *parse_atom(struct parser *p)
{
        if (p->current.type == LX_STRING)
                return parse_string_literal(p);

        if (p->current.type == LX_INTEGER || p->current.type == LX_DECIMAL)
                return parse_number(p);

        printf("Could not identify expression atom starting with %s (\"%s\") on line %ld.\n",
               lxtype_string(p->current.type), p->current.value, p->line);

        return NULL;
}

struct astnode *parse_number(struct parser *p)
{
        if (p->current.type != LX_INTEGER && p->current.type != LX_DECIMAL) {
                printf("Could not parse number. Expected literal, got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        double d = string_to_number(p->current.value);

        struct astnode *num = astnode_number_literal(p->line, d);

        parser_advance(p);

        return num;
}

struct astnode *parse_string_literal(struct parser *p)
{
        if (p->current.type != LX_STRING) {
                printf("Expected string literal, but found %s (\"%s\") on line %ld.\n", lxtype_string(p->current.type),
                       p->current.value, p->line);
                return NULL;
        }

        struct astnode *str = astnode_string_literal(p->line, p->current.value);;;

        parser_advance(p);

        return str;
}