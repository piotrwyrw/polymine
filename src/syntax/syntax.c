//
// Created by Piotr Krzysztof Wyrwas on 24.03.24.
//

#include "parser.h"
#include "syntax.h"
#include "../common/util.h"
#include "../semantics/semutil.h"

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
                if (strcmp(p->current.value, "resolve") == 0)
                        return parse_resolve(p);

                if (strcmp(p->current.value, "nothing") == 0)
                        return parse_nothing(p);

                if (strcmp(p->current.value, "var") == 0)
                        return parse_variable_declaration(p);

                if (strcmp(p->current.value, "if") == 0)
                        return parse_if(p);

                if (strcmp(p->current.value, "include") == 0)
                        return parse_include(p);

                if (strcmp(p->current.value, "present") == 0)
                        return parse_present(p);

                if (strcmp(p->current.value, "type") == 0)
                        return parse_type_definition(p);

                if (strcmp(p->current.value, "fn") == 0)
                        return parse_function_definition(p);
        }

        if (p->current.type == LX_LBRACE)
                return parse_block(p);

        struct astnode *expr = parse_expr(p);

        if (!expr)
                return NULL;

        // Assignment
        if (expr->type == NODE_PATH && p->current.type == LX_EQUALS) {
                parser_advance(p);

                struct astnode *value = parse_expr(p);

                if (!value) {
                        astnode_free(expr);
                        return NULL;
                }

                struct astnode *assignment = astnode_assignment(expr->line, p->block, expr, value);
                expr->holder = assignment;
                return assignment;
        }

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

struct astnode *parse_type_definition(struct parser *p)
{
        if (p->current.type != LX_IDEN || (strcmp(p->current.value, "type") != 0)) {
                printf("Expected 'type' identifier at the start of a type definition. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        parser_advance(p);

        if (p->current.type != LX_IDEN) {
                printf("Expected type identifier after 'type' keyword. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        char *id = strdup(p->current.value);

        parser_advance(p);

        struct astnode *fields = parse_parameters(p, true);

        if (!fields) {
                free(id);
                return NULL;
        }

        struct astnode *block = NULL;

        if (p->current.type == LX_LBRACE) {
                block = parse_block(p);
                if (!block) {
                        astnode_free(fields);
                        free(id);
                        return NULL;
                }
        }

        if (!block)
                block = astnode_empty_block(p->line, p->block);

        struct astnode *def = astnode_type_definition(p->line, p->block, id, fields, block);

        def->type_definition.block->holder = def;
        def->type_definition.fields->holder = def;

        free(id);

        return def;
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

        struct astdtype *type = NULL;

        if (p->current.type == LX_COLON) {
                parser_advance(p);

                if (p->current.type != LX_QUESTION_MARK) {
                        type = parse_type(p);

                        if (!type) {
                                free(id);
                                return NULL;
                        }
                } else
                        parser_advance(p);
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

struct astnode *parse_parameters(struct parser *p, _Bool defaultValues)
{
        if (p->current.type != LX_LPAREN) {
                printf("Expected '(' at the beginning of a parameter list. Got %s (\"%s\") on line %ld.\n",
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

                struct astnode *value = NULL;

                if (p->current.type == LX_IDEN && strcmp(p->current.value, "default") == 0) {
                        if (!defaultValues) {
                                printf("Default values are not allowed in a parameter list in this context. Error on line %ld.\n",
                                       p->line);
                                free(id);
                                astnode_free(params);
                                return NULL;
                        }

                        parser_advance(p);

                        value = parse_expr(p);

                        if (!value) {
                                free(id);
                                astnode_free(params);
                                return NULL;
                        }
                }

                struct astnode *declaration = astnode_declaration(p->line, p->block, false, id, type, value);
                declaration->holder = params;
                astnode_push_compound(params, declaration);

                free(id);

                if (p->current.type == LX_RPAREN)
                        break;

                if (p->current.type != LX_COMMA && p->next.type != LX_RPAREN) {
                        printf("Expected ')' at the end of parameter list, or ',' and more parameters. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        astnode_free(params);
                        return NULL;
                }

                parser_advance(p);
        }

        if (p->current.type != LX_RPAREN) {
                printf("Expected ')' after parameter list. Got %s (\"%s\") on line %ld.\n",
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
                printf("Expected 'fn' keyword at the start of a function definition. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        size_t line = p->line;

        struct astnode *attrs = NULL;

        parser_advance(p);

        if (p->current.type == LX_LSQUARE) {
                attrs = parse_attributes(p);
                if (!attrs)
                        return NULL;
        }

        if (!attrs)
                attrs = astnode_empty_compound(p->line, p->block);

        if (p->current.type != LX_IDEN) {
                printf("Expected function identifier after 'fn' keyword. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        char *id = strdup(p->current.value);

        parser_advance(p);

        struct astnode *params = NULL;

        if (p->current.type == LX_LPAREN) {
                params = parse_parameters(p, false);
                if (!params) {
                        free(id);
                        astnode_free(attrs);
                        return NULL;
                }
        } else
                params = astnode_empty_compound(p->line, p->block);

        if (p->current.type != LX_MOV_RIGHT) {
                printf("Expected '->' after function parameter block. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);
                astnode_free(attrs);
                astnode_free(params);
                return NULL;
        }

        parser_advance(p);

        struct astdtype *type = parse_type(p);

        if (!type) {
                free(id);
                astnode_free(attrs);
                astnode_free(params);
                return NULL;
        }

        struct astnode *block;
        struct astnode *fdef;

        // Expression-valued functions (just syntax sugar, really)
        if (p->current.type == LX_EQUALS) {
                parser_advance(p);

                block = astnode_empty_block(p->line, p->block);

                struct astnode *oldBlock = p->block;

                p->block = block;

                struct astnode *expr = parse_expr(p);

                if (!expr) {
                        free(id);
                        astnode_free(block);
                        astnode_free(attrs);
                        astnode_free(params);
                        return NULL;
                }

                struct astnode *resolve = astnode_resolve(p->line, p->block, expr);

                p->block = oldBlock;

                astnode_push_compound(block->block.nodes, resolve);

                goto finalize;
        }

        if (p->current.type != LX_LBRACE) {
                printf("Expected '{' after function parameter block pr '=' for an expression-valued function. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);
                astnode_free(attrs);
                astnode_free(params);
                return NULL;
        }

        block = parse_block(p);

        if (!block) {
                free(id);
                astnode_free(attrs);
                astnode_free(params);
                return NULL;
        }

        finalize:

        fdef = astnode_function_definition(line, p->block, id, params, type, block, attrs);
        fdef->function_def.params->holder = fdef;
        fdef->function_def.block->holder = fdef;

        free(id);

        return fdef;
}

struct astnode *parse_present(struct parser *p)
{
        if (p->current.type != LX_IDEN || strcmp(p->current.value, "present") != 0) {
                printf("Expected 'linked' at the beginning of a linked function definition. Got %s (\"%s\") error on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        parser_advance(p);

        if (p->current.type != LX_IDEN) {
                printf("Expected function identifier after linked keyword. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        char *id = strdup(p->current.value);

        parser_advance(p);

        struct astnode *params = parse_parameters(p, false);

        if (!params) {
                free(id);
                return NULL;
        }

        if (p->current.type != LX_MOV_RIGHT) {
                printf("Expected '->' after parameter block. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                free(id);
                return NULL;
        }

        parser_advance(p);

        struct astdtype *type = parse_type(p);

        if (!type) {
                astnode_free(params);
                free(id);
                return NULL;
        }

        struct astnode *linked = astnode_present_function(p->line, p->block, id, params, type);
        free(id);
        return linked;
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

        struct astnode *expr = parse_expr(p);
        if (!expr)
                return NULL;

        struct astnode *resv = astnode_resolve(line, p->block, expr);
        resv->resolve.value->holder = resv;
        return resv;
}

struct astnode *parse_if(struct parser *p)
{
        if (p->current.type != LX_IDEN || strcmp(p->current.value, "if") != 0) {
                printf("Expected 'if' at the start of an if-statement. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        parser_advance(p);

        struct astnode *expr = parse_expr(p);

        if (!expr)
                return NULL;

        struct astnode *block = parse_block(p);

        if (!block) {
                astnode_free(expr);
                return NULL;
        }

        struct astnode *base = astnode_if(p->line, p->block, expr, block, NULL);

        block->holder = base;

        struct astnode *branch_tip = base;

        while (p->current.type == LX_IDEN && strcmp(p->current.value, "else") == 0) {
                struct astnode *branch;

                parser_advance(p);

                struct astnode *branch_condition = NULL;
                struct astnode *branch_block;

                if (p->current.type == LX_IDEN && strcmp(p->current.value, "if") == 0) {
                        parser_advance(p);
                        branch_condition = parse_expr(p);
                        if (!branch_condition) {
                                astnode_free(base);
                                return NULL;
                        }
                }

                branch_block = parse_block(p);

                if (!branch_block) {
                        astnode_free(branch_condition);
                        astnode_free(base);
                        return NULL;
                }

                branch = astnode_if(p->line, p->block, branch_condition, branch_block, NULL);

                branch_block->holder = branch;

                branch_tip->if_statement.next_branch = branch;
                branch_tip = branch;

                // The branch condition being NULL implies that this is the final branch (ELSE)
                // Thus, we don't need to look any further for more branches.
                if (!branch_condition)
                        break;
        }

        return base;
}

struct astnode *parse_include(struct parser *p)
{
        if (p->current.type != LX_IDEN || strcmp(p->current.value, "include") != 0) {
                printf("Expected 'include' at the start of an include directive. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        parser_advance(p);

        if (p->current.type != LX_STRING) {
                printf("Expected string literal after the include identifier. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        struct astnode *include = astnode_include(p->line, p->block, p->current.value);
        parser_advance(p);
        return include;
}

struct astnode *parse_attributes(struct parser *p)
{
        if (p->current.type != LX_LSQUARE) {
                printf("Expected '[' at the start of an attribute list, got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        parser_advance(p);

        struct astnode *attrs = astnode_empty_compound(p->line, p->block);

        while (p->current.type != LX_RSQUARE) {
                if (p->current.type != LX_IDEN) {
                        printf("Expected a comma-separated list of attributes, got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        astnode_free(attrs);
                        return NULL;
                }

                if (has_attribute(attrs, p->current.value)) {
                        printf("An attribute cannot be inserted into the list more than once. Error on line %ld for attribute \"%s\".\n",
                               p->current.line, p->current.value);
                        return NULL;
                }

                astnode_push_compound(attrs, astnode_attribute(p->line, p->block, p->current.value));

                parser_advance(p);

                if (p->current.type == LX_COMMA) {
                        parser_advance(p);
                        continue;
                }

                break;
        }

        if (p->current.type != LX_RSQUARE) {
                printf("Expected ']' at the end of an attribute list, got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                astnode_free(attrs);
                return NULL;
        }

        parser_advance(p);

        return attrs;
}

static struct astdtype *actually_parse_type(struct parser *p)
{
        char *identifier;

        if (p->current.type != LX_IDEN) {
                printf("Expected type name or functional identifier. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        if (strcmp(p->current.value, "void") == 0) {
                parser_advance(p);

                struct astdtype *v = astdtype_void();

                return v;
        }

        identifier = p->current.value;

        enum builtin_type bt = builtin_from_string(identifier);

        if (bt != BUILTIN_UNDEFINED) {
                parser_advance(p);

                struct astdtype *builtin = astdtype_builtin(bt);

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

                return pointer;
        }

        struct astdtype *type = astdtype_complex(identifier);
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

        while (p->current.type == LX_PLUS || p->current.type == LX_MINUS || p->current.type == LX_DOUBLE_AND ||
               p->current.type == LX_DOUBLE_OR) {
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
        struct astnode *left = parse_atom_front(p);

        if (!left)
                return NULL;

        while (p->current.type == LX_ASTERISK || p->current.type == LX_SLASH || p->current.type == LX_LGREATER ||
               p->current.type == LX_LGREQUAL || p->current.type == LX_RGREATER || p->current.type == LX_RGREQUAL
               || p->current.type == LX_DOUBLE_EQUALS) {

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

struct astnode *parse_atom_front(struct parser *p)
{
        struct astnode *root = parse_atom(p);

        if (!root)
                return NULL;

        if (p->current.type != LX_DOT)
                return root;

        root = astnode_path(p->line, p->block, root);
        struct astnode *tip = root;

        while (p->current.type == LX_DOT) {
                parser_advance(p);

                struct astnode *nextExpr = parse_atom(p);

                if (!nextExpr) {
                        astnode_free(root);
                        return NULL;
                }

                tip->path.next = astnode_path(p->line, p->block, nextExpr);

                tip = tip->path.next;
        }

        return root;
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

        if (p->current.type == LX_IDEN && strcmp(p->current.value, "true") == 0) {
                parser_advance(p);
                return astnode_integer_literal(p->line, p->block, 1);
        }

        if (p->current.type == LX_IDEN && strcmp(p->current.value, "false") == 0) {
                parser_advance(p);
                return astnode_integer_literal(p->line, p->block, 0);
        }

        if (p->current.type == LX_IDEN && (strcmp(p->current.value, "nothing") == 0)) {
                parser_advance(p);
                return astnode_void_placeholder(p->line, p->block);
        }

        if (p->current.type == LX_IDEN &&
            (strcmp(p->current.value, "ptr_to") == 0 || strcmp(p->current.value, "deref") == 0)) {
                _Bool pointer = (strcmp(p->current.value, "ptr_to") == 0);

                char *keyword = strdup(p->current.value);

                size_t line = p->line;

                parser_advance(p);

                if (p->current.type != LX_LSQUARE) {
                        printf("Expected '[' after '%s' keyword. Got %s (\"%s\") on line %ld.\n",
                               keyword, lxtype_string(p->current.type), p->current.value, p->line);
                        free(keyword);
                        return NULL;
                }

                free(keyword);

                parser_advance(p);

                struct astnode *to = parse_expr(p);

                if (!to)
                        return NULL;

                if (p->current.type != LX_RSQUARE) {
                        printf("Expected ']' after expression. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        return NULL;
                }

                parser_advance(p);

                struct astnode *expr;

                if (pointer)
                        expr = astnode_pointer(line, p->block, to);
                else
                        expr = astnode_dereference(line, p->block, to);

                return expr;
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