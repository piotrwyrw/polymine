//
// Created by Piotr Krzysztof Wyrwas on 24.03.24.
//

#include "parser.h"
#include "syntax.h"
#include "util.h"

#include <string.h>

struct astnode *parse(struct parser *p)
{
        struct astnode *program = astnode_empty_program();

        program->program.block = parse_block_advanced(p, false);

        if (!program->program.block)
                return NULL;

        return program;
}

static struct astnode *parser_parse_whatever(struct parser *p, enum pstatus *status)
{
        *status = PARSER_SYNTAX_ERROR;

        if (p->current.type == LX_UNDEFINED)
                goto exit_no_match;

        if (p->current.type == LX_STRING)
                return parse_string_literal(p);

        if (p->current.type == LX_IDEN) {
                if (p->next.type == LX_EQUALS)
                        return parse_variable_assignment(p);

                if (strcmp(p->current.value, "nothing") == 0)
                        return parse_nothing(p);

                if (strcmp(p->current.value, "var") == 0)
                        return parse_variable_declaration(p);

                if (strcmp(p->current.value, "func") == 0)
                        return parse_function_definition(p);
        }

        if (p->current.type == LX_LBRACE)
                return parse_block(p);

        return parse_expr(p);

        exit_no_match:
        *status = PARSER_NO_MATCH;
        return NULL;
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

struct astnode *parse_variable_declaration(struct parser *p)
{
        if (p->current.type != LX_IDEN || strcmp(p->current.value, "var") != 0) {
                printf("Expected 'var' at the beginning of a variable declaration. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        size_t line = p->line;
        _Bool constant = false;

        parser_advance(p);

        if (p->current.type == LX_IDEN && strcmp(p->current.value, "stable") == 0) {
                constant = true;
                parser_advance(p);
        }

        if (p->current.type != LX_IDEN) {
                printf("Expected variable identifier. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        char *id = strdup(p->current.value);

        parser_advance(p);

        if (p->current.type != LX_COLON) {
                printf("Expected ':' after variable identifier \"%s\". Got %s (\"%s\") on line %ld.\n", id,
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);
                return NULL;
        }

        parser_advance(p);

        struct astdtype *type = parse_type(p);

        if (!type) {
                free(id);
                return NULL;
        }

        struct astnode *expr = NULL;

        if (p->current.type == LX_EQUALS) {
                parser_advance(p);

                expr = parse_expr(p);

                if (!expr) {
                        free(id);
                        return NULL;
                }
        }

        if (p->current.type != LX_SEMI) {
                printf("Expected ';' after variable declaration. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);

                astnode_free(expr);

                return NULL;
        }

        parser_advance(p);

        struct astnode *decl = astnode_declaration(line, constant, id, type, expr);
        free(id);
        return decl;
}

struct astnode *parse_variable_assignment(struct parser *p)
{
        size_t line = p->line;

        if (p->current.type != LX_IDEN) {
                printf("Expectesd identifier on the left side of a variable assignment. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, line);
                return NULL;
        }

        char *id = strdup(p->current.value);

        parser_advance(p);

        if (p->current.type != LX_EQUALS) {
                printf("Expected '=' after variable identifier. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, line);
                free(id);
                return NULL;
        }

        parser_advance(p);

        struct astnode *val = parse_expr(p);

        if (!val) {
                free(id);
                return NULL;
        }

        if (p->current.type != LX_SEMI) {
                printf("Expected ';' after variable assignment. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);
                return NULL;
        }

        parser_advance(p);

        struct astnode *assignment = astnode_assignment(line, id, val);

        free(id);

        return assignment;
}

struct astnode *parse_function_parameters(struct parser *p)
{
        if (p->current.type != LX_LPAREN) {
                printf("Expected '(' at the beginning of a function parameter list. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        parser_advance(p);

        struct astnode *params = astnode_empty_block(p->line);

        while (p->current.type != LX_UNDEFINED) {
                if (p->current.type == LX_RPAREN)
                        break;

                char *id;
                struct astdtype *type;

                if (p->current.type != LX_IDEN) {
                        printf("Expected parameter identifier. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        astnode_free(params);
                        return NULL;
                }

                id = strdup(p->current.value);

                parser_advance(p);

                if (p->current.type != LX_COLON) {
                        printf("Expected ':' after parameter identifier. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        free(id);
                        astnode_free(params);
                        return NULL;
                }

                parser_advance(p);

                type = parse_type(p);

                if (!type) {
                        free(id);
                        astdtype_free(type);
                        astnode_free(params);
                        return NULL;
                }

                astnode_push_block(params, astnode_declaration(p->line, false, id, type, NULL));

                free(id);

                if (p->current.type == LX_RPAREN)
                        break;

                if (p->current.type != LX_COMMA && p->next.type != LX_RPAREN) {
                        printf("Expected ')' at the end of function parameter list, or ',' and more parameters. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        astnode_free(params);
                        return NULL;
                }

                parser_advance(p);
        }

        if (p->current.type != LX_RPAREN) {
                printf("Expected ')' after function parameter list. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                astnode_free(params);
                return NULL;
        }

        parser_advance(p);

        return params;
}

struct astnode *parse_function_definition(struct parser *p)
{
        if (p->current.type != LX_IDEN) {
                printf("Expected 'func' keyword at the start of a function definition. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        size_t line = p->line;

        parser_advance(p);

        if (p->current.type != LX_IDEN) {
                printf("Expected function identifier after func keyword. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        char *id = strdup(p->current.value);

        parser_advance(p);

        struct astnode *params = NULL;

        if (p->current.type == LX_LPAREN) {
                params = parse_function_parameters(p);
                if (!params) {
                        free(id);
                        return NULL;
                }
        } else
                params = astnode_empty_block(p->line);

        if (p->current.type != LX_IDEN || strcmp(p->current.value, "of") != 0) {
                printf("Expected 'of' after function parameter block. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);
                return NULL;
        }

        parser_advance(p);

        struct astdtype *type = parse_type(p);

        if (!type) {
                free(id);
                astnode_free(params);
                return NULL;
        }

        if (p->current.type != LX_LBRACE) {
                printf("Expected '{' after function parameter block. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);
                astnode_free(params);
                return NULL;
        }

        struct astnode *block = parse_block(p);

        if (!block) {
                free(id);
                astnode_free(params);
                return NULL;
        }

        struct astnode *fdef = astnode_function_definition(line, id, params, type, block);

        free(id);

        return fdef;
}

struct astdtype *parse_type(struct parser *p)
{
        char *identifier;

        if (p->current.type != LX_IDEN) {
                printf("Expected type name or functional identifier. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        if (strcmp(p->current.value, "void") == 0) {
                parser_advance(p);
                return astdtype_void();
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
        if (p->current.type == LX_LPAREN) {
                parser_advance(p);
                struct astnode *expr = parse_expr(p);

                if (!expr)
                        return NULL;

                if (p->current.type != LX_RPAREN) {
                        printf("Expected ')' after sub-expression. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        astnode_free(expr);
                        return NULL;
                }

                parser_advance(p);

                return expr;
        }

        if (p->current.type == LX_STRING)
                return parse_string_literal(p);

        if (p->current.type == LX_INTEGER || p->current.type == LX_DECIMAL)
                return parse_number(p);

        if (p->current.type == LX_IDEN && p->next.type == LX_LPAREN)
                return parse_function_call(p);

        if (p->current.type == LX_IDEN && strcmp(p->current.value, "ptr_to") == 0) {
                size_t line = p->line;

                parser_advance(p);

                struct astnode *to = parse_expr(p);

                if (!to)
                        return NULL;

                return astnode_pointer(line, to);
        }

        if (p->current.type == LX_IDEN) {
                struct astnode *var = astnode_variable(p->line, p->current.value);
                parser_advance(p);
                return var;
        }

        printf("Could not identify expression atom starting with %s (\"%s\") on line %ld.\n",
               lxtype_string(p->current.type), p->current.value, p->line);

        return NULL;
}

struct astnode *parse_function_call(struct parser *p)
{
        if (p->current.type != LX_IDEN) {
                printf("Expected identifier at the start of a function call. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        size_t line = p->line;
        char *id = strdup(p->current.value);

        parser_advance(p);

        if (p->current.type != LX_LPAREN) {
                printf("Expected '(' after function identifier. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);
                return NULL;
        }

        parser_advance(p);

        struct astnode *values = astnode_empty_block(line);
        struct astnode *expr;

        while (p->current.type != LX_UNDEFINED) {
                if (p->current.type == LX_RPAREN)
                        break;

                expr = parse_expr(p);

                if (!expr) {
                        free(id);
                        astnode_free(values);
                        return NULL;
                }

                astnode_push_block(values, expr);

                if (p->current.type == LX_RPAREN)
                        break;

                if (p->current.type != LX_COMMA && p->next.type != LX_RPAREN) {
                        printf("Expected ')' or ',' and more values. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        free(id);
                        astnode_free(values);
                        return NULL;
                }

                if (p->current.type == LX_COMMA)
                        parser_advance(p);
        }

        if (p->current.type != LX_RPAREN) {
                printf("Expected ')' at the end of a function call value list. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);
                astnode_free(values);
                return NULL;
        }

        parser_advance(p);

        struct astnode *call = astnode_function_call(line, id, values);

        free(id);

        return call;
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