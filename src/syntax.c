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

        if (!program->program.block) {
                free(program);
                return NULL;
        }

        program->program.block->holder = program;

        return program;
}

static struct astnode *parser_parse_whatever(struct parser *p, enum pstatus *status)
{
        *status = PARSER_SYNTAX_ERROR;

        // Redundant semicolons are naughty, but we won't complain
        if (p->current.type == LX_SEMI) {
                size_t line = p->line;

                printf("Warning: Redundant semicolon on line %ld.\n", line);

                parser_advance(p);

                return astnode_nothing(line, p->block);
        }

        if (p->current.type == LX_UNDEFINED)
                goto exit_no_match;

        if (p->current.type == LX_STRING)
                return parse_string_literal(p);

        if (p->current.type == LX_IDEN) {
                if (p->next.type == LX_EQUALS)
                        return parse_variable_assignment(p);

                if (strcmp(p->current.value, "resolve") == 0)
                        return parse_resolve(p);

                if (strcmp(p->current.value, "nothing") == 0)
                        return parse_nothing(p);

                if (strcmp(p->current.value, "var") == 0)
                        return parse_variable_declaration(p);
        }

        if (p->current.type == LX_LBRACE)
                return parse_block(p);

        struct astnode *expr = parse_expr(p);

        return expr;

        exit_no_match:
        *status = PARSER_NO_MATCH;
        return NULL;
}

struct astnode *parse_block_very_advanced(struct parser *p, _Bool decorated, struct astnode *block)
{
        if (decorated) {
                if (p->current.type != LX_LBRACE) {
                        printf("Expected '{' at the beginning of a block. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        return NULL;
                }

                parser_advance(p);
        }

        struct astnode *node;
        enum pstatus status;

        struct astnode *oldBlock = p->block;

        // All nodes inside this block will refer to the new block throughout its lifetime
        p->block = block;

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

                astnode_push_compound(block->block.nodes, node);
        }

        if (decorated) {
                if (p->current.type != LX_RBRACE) {
                        printf("Expected '}' at the end of a block statement. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        return NULL;
                }

                parser_advance(p);
        }

        // Go back to the old block
        p->block = oldBlock;

        return block;

        syntax_error:

        p->block = oldBlock;

        return NULL;
}

struct astnode *parse_block_advanced(struct parser *p, _Bool decorated)
{
        struct astnode *block = astnode_empty_block(p->line, p->block);
        if (!parse_block_very_advanced(p, decorated, block)) {
                astnode_free(block);
                return NULL;
        }
        return block;
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

        return astnode_nothing(line, p->block);
}

struct astnode *parse_variable_declaration(struct parser *p)
{
        if (p->current.type != LX_IDEN || (p->current.type == LX_IDEN && strcmp(p->current.value, "var") != 0)) {
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

        struct astnode *decl = astnode_declaration(line, p->block, constant, id, type, expr);

        if (decl->declaration.value)
                decl->declaration.value->holder = decl;

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

        struct astnode *assignment = astnode_assignment(line, p->block, id, val);

        assignment->assignment.value->holder = assignment;

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

        struct astnode *params = astnode_empty_compound(p->line, p->block);

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
                        astnode_free(params);
                        return NULL;
                }

                astnode_push_compound(params,
                                      astnode_declaration(p->line, p->block, false, id, type, NULL));

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
                printf("Expected 'func' or 'anon' keyword at the start of a function definition. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        _Bool anon = strcmp(p->current.value, "anon") == 0;
        size_t line = p->line;

        parser_advance(p);

        if (p->current.type != LX_IDEN && !anon) {
                printf("Expected function identifier after func keyword. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        char *id = anon ? NULL : strdup(p->current.value);

        if (!anon)
                parser_advance(p);

        struct astnode *params = NULL;

        if (p->current.type == LX_LPAREN) {
                params = parse_function_parameters(p);
                if (!params) {
                        free(id);
                        return NULL;
                }
        } else
                params = astnode_empty_compound(p->line, p->block);

        if (p->current.type != LX_IDEN || strcmp(p->current.value, "of") != 0) {
                printf("Expected 'of' after function parameter block. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);
                astnode_free(params);
                return NULL;
        }

        parser_advance(p);

        struct astdtype *type = parse_type(p);

        if (!type) {
                free(id);
                astnode_free(params);
                return NULL;
        }

        struct astnode *capture = NULL;

        if (p->current.type == LX_IDEN) {
                if (strcmp(p->current.value, "captures") != 0) {
                        printf("Expected 'captures' followed by a capture group, or the function block after ther type. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        free(id);
                        astnode_free(params);
                        return NULL;
                }

                parser_advance(p);

                capture = astnode_empty_compound(p->line, p->block);

                while (p->current.type != LX_UNDEFINED) {
                        if (p->current.type != LX_IDEN) {
                                printf("Expected identifier after the captures keyword. Got %s (\"%s\") on line %ld.\n",
                                       lxtype_string(p->current.type), p->current.value, p->line);
                                free(id);
                                astnode_free(params);
                                astnode_free(capture);
                                return NULL;
                        }

                        astnode_push_compound(capture,
                                              astnode_declaration(p->line, p->block, true, p->current.value, NULL,
                                                                  NULL));

                        parser_advance(p);

                        if (p->current.type == LX_COMMA)
                                parser_advance(p);

                        if (p->current.type == LX_LBRACE)
                                break;
                        else if (p->current.type == LX_IDEN)
                                continue;

                        if (p->current.type != LX_COMMA && p->current.type != LX_LBRACE) {
                                printf("Expected ',' and more capture variables, or the function block. Got %s (\"%s\") on line %ld.\n",
                                       lxtype_string(p->current.type), p->current.value, p->line);
                                free(id);
                                astnode_free(params);
                                astnode_free(capture);
                                return NULL;
                        }
                }
        }

        if (p->current.type != LX_LBRACE) {
                printf("Expected '{' after function parameter block. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);
                astnode_free(params);
                astnode_free(capture);
                return NULL;
        }

        struct astnode *block = parse_block(p);

        if (!block) {
                free(id);
                astnode_free(params);
                astnode_free(capture);
                return NULL;
        }

        struct astnode *fdef = astnode_function_definition(line, p->block, id, params, type, capture, block);

        fdef->function_def.params->holder = fdef;
        fdef->function_def.block->holder = fdef;

        if (fdef->function_def.capture)
                fdef->function_def.capture->holder = fdef;

        free(id);

        return fdef;
}

struct astnode *parse_resolve(struct parser *p)
{
        if (p->current.type != LX_IDEN || (p->current.type == LX_IDEN && strcmp(p->current.value, "resolve") != 0)) {
                printf("Expected 'resolve' at the beginning of a resolution statement. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        size_t line = p->line;

        parser_advance(p);

        struct astnode *resv = astnode_resolve(line, p->block, parse_expr(p));
        resv->resolve.value->holder = resv;

        return resv;
}

static struct astdtype *actually_parse_type(struct parser *p)
{
        char *identifier;

        if (p->current.type != LX_IDEN) {
                printf("Expected type name or functional identifier. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        if (strcmp(p->current.value, "lambda") == 0) {
                parser_advance(p);

                if (p->current.type != LX_LPAREN) {
                        printf("Expected '(' after function_def type. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        return NULL;
                }

                parser_advance(p);

                struct astnode *pTypes = astnode_empty_compound(p->line, p->block);

                while (p->current.type != LX_UNDEFINED) {
                        if (p->current.type == LX_RPAREN) {
                                parser_advance(p);
                                break;
                        }

                        if (p->current.type != LX_IDEN) {
                                printf("Expected lambda parameter type. Got %s (\"%s\") on line %ld.\n",
                                       lxtype_string(p->current.type), p->current.value, p->line);
                                astnode_free(pTypes);
                                return NULL;
                        }

                        struct astdtype *type = parse_type(p);

                        if (!type) {
                                astnode_free(pTypes);
                                return NULL;
                        }

                        astnode_push_compound(pTypes, astnode_data_type(type));

                        if (p->current.type == LX_COMMA) {
                                parser_advance(p);
                                continue;
                        }

                        if (p->current.type == LX_RPAREN) {
                                parser_advance(p);
                                break;
                        }

                        printf("Expected ',' and more lambda parameter types. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);

                        astnode_free(pTypes);

                        return NULL;
                }

                if (p->current.type != LX_MOV_RIGHT) {
                        printf("Expected '->' after lambda parameters. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);

                        astnode_free(pTypes);

                        return NULL;
                }

                parser_advance(p);

                struct astdtype *type = parse_type(p);

                if (!type) {
                        astnode_free(pTypes);
                        return NULL;
                }

                struct astdtype *lambda = astdtype_lambda(pTypes, type);

//                astnode_push_compound(p->types, astnode_data_type(lambda));

                return lambda;
        }

        if (strcmp(p->current.value, "void") == 0) {
                parser_advance(p);

                struct astdtype *v = astdtype_void();
//                astnode_push_compound(p->types, astnode_data_type(v));

                return v;
        }

        identifier = p->current.value;

        enum builtin_type bt = builtin_from_string(identifier);

        if (bt != BUILTIN_UNDEFINED) {
                parser_advance(p);

                struct astdtype *builtin = astdtype_builtin(bt);
//                astnode_push_compound(p->types, astnode_data_type(builtin));

                return builtin;
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
                        return NULL;
                }

                parser_advance(p);

                struct astdtype *pointer = astdtype_pointer(enclosed);
//                astnode_push_compound(p->types, astnode_data_type(pointer));

                return pointer;
        }

        struct astdtype *type = astdtype_custom(identifier);
        parser_advance(p);

        return type;
}


struct astdtype *parse_type(struct parser *p)
{
        struct astdtype *type = actually_parse_type(p);
        if (!type)
                return NULL;
        astnode_push_compound(p->types, astnode_data_type(type));
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

                left = astnode_binary(line, p->block, left, right, op);
                left->binary.left->holder = left;
                left->binary.right->holder = left;
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

                left = astnode_binary(line, p->block, left, right, op);
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

        if (p->current.type == LX_IDEN && (strcmp(p->current.value, "func") == 0 || strcmp(p->current.value, "anon") == 0))
                return parse_function_definition(p);

        if (p->current.type == LX_STRING)
                return parse_string_literal(p);

        if (p->current.type == LX_INTEGER || p->current.type == LX_DECIMAL)
                return parse_number(p);

        if (p->current.type == LX_IDEN && p->next.type == LX_LPAREN)
                return parse_function_call(p);

        if (p->current.type == LX_IDEN && strcmp(p->current.value, "true") == 0) {
                parser_advance(p);
                return astnode_integer_literal(p->line, p->block, 1);
        }

        if (p->current.type == LX_IDEN && strcmp(p->current.value, "false") == 0) {
                parser_advance(p);
                return astnode_integer_literal(p->line, p->block, 0);
        }

        if (p->current.type == LX_IDEN && strcmp(p->current.value, "ptr_to") == 0) {
                size_t line = p->line;

                parser_advance(p);

                if (p->current.type != LX_RGREATER) {
                        printf("Expected '<' after 'ptr_to' keyword. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        return NULL;
                }

                parser_advance(p);

                struct astnode *to = parse_expr(p);

                if (!to)
                        return NULL;

                if (p->current.type != LX_LGREATER) {
                        printf("Expected '>' after pointer expression. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        return NULL;
                }

                parser_advance(p);

                return astnode_pointer(line, p->block, to);
        }

        if (p->current.type == LX_IDEN) {
                struct astnode *var = astnode_variable(p->line, p->block, p->current.value);
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

        struct astnode *values = astnode_empty_compound(line, p->block);
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

                astnode_push_compound(values, expr);

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

        struct astnode *call = astnode_function_call(line, p->block, id, values);
        call->function_call.values->holder = call;

        free(id);

        return call;
}

struct astnode *parse_number(struct parser *p)
{
        if (p->current.type != LX_INTEGER && p->current.type != LX_DECIMAL) {
                printf("Could not parse number. Expected integer literal or float literal, got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        enum lxtype type = p->current.type;

        struct astnode *n;

        if (type == LX_INTEGER)
                n = astnode_integer_literal(p->line, p->block, string_to_integer(p->current.value));
        else
                n = astnode_float_literal(p->line, p->block, string_to_float(p->current.value));

        parser_advance(p);

        return n;
}

struct astnode *parse_string_literal(struct parser *p)
{
        if (p->current.type != LX_STRING) {
                printf("Expected string literal, but found %s (\"%s\") on line %ld.\n", lxtype_string(p->current.type),
                       p->current.value, p->line);
                return NULL;
        }

        struct astnode *str = astnode_string_literal(p->line, p->block, p->current.value);

        parser_advance(p);

        return str;
}