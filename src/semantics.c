#include "semantics.h"
#include "semutil.h"

#include <stdbool.h>
#include <string.h>

// TODO Analyze capture groups, introduce captured variables as function parameters

_Bool analyze_program(struct semantics *sem, struct astnode *program)
{
        return analyze_any(sem, program->program.block);
}

_Bool analyze_compound(struct semantics *sem, struct astnode *compound)
{
        _Bool success = true;

        for (size_t i = 0; i < compound->node_compound.count; i++)
                if (!analyze_any(sem, compound->node_compound.array[i]))
                        success = false;

        return success;
}

_Bool analyze_any(struct semantics *sem, struct astnode *node)
{
        switch (node->type) {
                case NODE_BLOCK:
                        return analyze_compound(sem, node->block.nodes);
                case NODE_VARIABLE_DECL:
                        return analyze_variable_declaration(sem, node);
                case NODE_FUNCTION_DEFINITION:
                        return analyze_function_definition(sem, node);
                case NODE_BINARY_OP:
                case NODE_INTEGER_LITERAL:
                case NODE_STRING_LITERAL:
                case NODE_FLOAT_LITERAL:
                case NODE_VARIABLE_USE:
                        return analyze_expression(sem, node) != NULL;
                case NODE_RESOLVE:
                        return analyze_resolve(sem, node);
                case NODE_NOTHING:
                        return true;
                default:
                        printf("Unknown node type passed to analyze_any(..): %s\n", nodetype_string(node->type));
                        return false;
        }
}

_Bool analyze_variable_declaration(struct semantics *sem, struct astnode *decl)
{
        if (symbol_conflict(decl->declaration.identifier, decl))
                return false;

        if (!decl->declaration.value)
                goto put_and_exit;

        struct astdtype *exprType = analyze_expression(sem, decl->declaration.value);

        if (!exprType) {
                printf("A variable of this type can not be created on line %ld.\n", decl->line);
                return false;
        }

        if (types_compatible(decl->declaration.type, exprType)) {
                printf("The effective type of the expression is not compatible with the variable declaration type on line %ld.\n",
                       decl->line);
                return false;
        }

        put_and_exit:
        put_symbol(decl->super,
                   astnode_symbol(decl->super, SYMBOL_VARIABLE, decl->declaration.identifier, decl->declaration.type,
                                  decl));

        return true;
}

static void *declare_param_variable(struct astnode *fdef, struct astnode *variable)
{
        put_symbol(fdef->function_def.block,
                   astnode_symbol(fdef->function_def.block, SYMBOL_VARIABLE, variable->declaration.identifier,
                                  variable->declaration.type, variable));
        return NULL;
}

_Bool analyze_function_definition(struct semantics *sem, struct astnode *fdef)
{
        if (symbol_conflict(fdef->function_def.identifier, fdef))
                return false;

        // Check if the function is allowed to have a capture group
        if (!find_enclosing_function(fdef->super) && fdef->function_def.capture) {
                printf("The function \"%s\" is not nested, and is therefore not allowed to have a capture group. Error on line %ld.\n",
                       fdef->function_def.identifier, fdef->line);
                return false;
        }

        astnode_compound_foreach(fdef->function_def.params->block.nodes, fdef, (void *) declare_param_variable);

        if (fdef->function_def.capture)
                analyze_capture_group(sem, fdef);

        if (!analyze_any(sem, fdef->function_def.block))
                return false;

        put_symbol(fdef->super,
                   astnode_symbol(fdef->super, SYMBOL_FUNCTION, fdef->function_def.identifier, fdef->function_def.type,
                                  fdef));

        return true;
}

static void *analyze_capture_variable(struct astnode *fdef, struct astnode *var)
{
        struct astnode *symbol;

        if (!(symbol = find_symbol(var->declaration.identifier, fdef->super))) {
                printf("The capture group variable \"%s\" is undefined. Error on line %ld.\n",
                       var->declaration.identifier,
                       var->line);
                return var;
        }

        // The function parameters have priority. See if our capture group variable naming conflicts with any params
        if (find_symbol_shallow(var->declaration.identifier, fdef->function_def.block)) {
                printf("Capture group variable \"%s\" conflicts with a function parameter of the same name. Error on line %ld.\n",
                       var->declaration.identifier, var->line);
                return var;
        }

        var->declaration.type = symbol->symbol.type;
        declare_param_variable(fdef, var);

        return NULL;
}

_Bool analyze_capture_group(struct semantics *sem, struct astnode *fdef)
{
        return astnode_compound_foreach(fdef->function_def.capture->block.nodes, fdef,
                                        (void *) analyze_capture_variable) == NULL;
}

_Bool analyze_resolve(struct semantics *sem, struct astnode *res)
{
        struct astdtype *type = analyze_expression(sem, res->resolve.value);
        struct astnode *function;

        if (!type)
                return false;

        if (!(function = find_enclosing_function(res->super))) {
                printf("A resolve statement may only be placed inside of a function. Violation on line %ld.\n",
                       res->line);
                return false;
        }

        if (!types_compatible(function->function_def.type, type)) {
                printf("The resolve statement expression is not compatible with the function type on line %ld.\n",
                       res->line);
                return false;
        }

        res->resolve.function = function->function_def.identifier;

        return true;
}

struct astdtype *analyze_expression(struct semantics *sem, struct astnode *expr)
{
        switch (expr->type) {
                case NODE_BINARY_OP:
                        return analyze_binary_expression(sem, expr);
                case NODE_VARIABLE_USE:
                        return analyze_variable_use(sem, expr);
                case NODE_INTEGER_LITERAL:
                case NODE_FLOAT_LITERAL:
                case NODE_STRING_LITERAL:
                        return analyze_atom(sem, expr);
                default:
                        return NULL;
        }
}

struct astdtype *analyze_binary_expression(struct semantics *sem, struct astnode *bin)
{
        struct astdtype *right = analyze_expression(sem, bin->binary.left);
        struct astdtype *left = analyze_expression(sem, bin->binary.right);

        if (!left || !right)
                return NULL;

        struct astdtype *type = required_type(left, right);

        if (!type) {
                printf("Cannot perform binary operation between the given types on line %ld.\n", bin->line);
                return NULL;
        }

        return type;
}

struct astdtype *analyze_variable_use(struct semantics *sem, struct astnode *use)
{
        struct astnode *symbol;

        if (!(symbol = find_symbol(use->variable.identifier, use->super))) {
                printf("Undefined symbol '%s' referenced on line %ld.\n", use->variable.identifier, use->line);
                return NULL;
        }

        if (symbol->symbol.symtype != SYMBOL_VARIABLE) {
                printf("Non-variable symbol '%s' (%s) referenced in variable context on line %ld.\n",
                       use->variable.identifier,
                       symbol_type_humanstr(symbol->symbol.symtype), use->line);
                return NULL;
        }

        return symbol->symbol.type;
}

struct astdtype *analyze_atom(struct semantics *sem, struct astnode *atom)
{
        if (atom->type == NODE_INTEGER_LITERAL)
                return required_type_integer(sem, atom->integer_literal.integerValue);

        if (atom->type == NODE_FLOAT_LITERAL)
                return sem->_double;

        if (atom->type == NODE_STRING_LITERAL)
                return sem->int64;

        return 0;
}