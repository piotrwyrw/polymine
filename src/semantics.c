#include "semantics.h"
#include "semutil.h"

#include <stdbool.h>
#include <string.h>

// ---------
// TODO Recurse through all blocks and scan stuff
// TODO FUNCTIONS !! Check if they're re-defined, handle nesting (?), allow for variable shadowing
// TODO Also, declare function variables as symbols in the scope!
// TODO Do some basic type correctness checking
// ---------

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
                default:
                        printf("Unknown node type passed to analyze_any(..): %s\n", nodetype_string(node->type));
                        return true;
        }
}

_Bool analyze_variable_declaration(struct semantics *sem, struct astnode *decl)
{
        struct astnode *symbol;

        if ((symbol = find_symbol(decl->declaration.identifier, decl->super))) {
                printf("Redefinition of symbol '%s' on line %ld. Original definition on line %ld as a %s.\n",
                       decl->declaration.identifier, decl->line, symbol->symbol.node->line,
                       symbol_type_humanstr(symbol->symbol.symtype));
                return false;
        }

        if (!decl->declaration.value)
                goto put_and_exit;

        struct astdtype *exprType = analyze_expression(sem, decl->declaration.value);

        if (!exprType) {
                printf("A variable of this type can not be created on line %ld.\n", decl->line);
                return false;
        }

        struct astdtype *req = required_type(exprType, decl->declaration.type);

        size_t expressionSize = quantify_type_size(req);
        size_t variableSize = quantify_type_size(decl->declaration.type);

        if (expressionSize > variableSize) {
                printf("The type size %ld of variable '%s' is too small to accommodate an expression that requires at least %ld bytes.\n",
                       variableSize, decl->declaration.identifier, expressionSize);
                return false;
        }

        put_and_exit:
        put_symbol(decl->super,
                   astnode_symbol(decl->super, SYMBOL_VARIABLE, decl->declaration.identifier, decl->declaration.type,
                                  decl));

        return true;
}

void *declare_param_variable(struct astnode *fdef, struct astnode *variable)
{
        put_symbol(fdef->function_def.block,
                   astnode_symbol(fdef->function_def.block, SYMBOL_VARIABLE, variable->declaration.identifier,
                                  variable->declaration.type, variable));
        return NULL;
}

_Bool analyze_function_definition(struct semantics *sem, struct astnode *fdef)
{
        struct astnode *symbol;

        if ((symbol = find_symbol(fdef->function_def.identifier, fdef->super))) {
                printf("The symbol '%s' is already defined - Redefinition attempted on line %ld. Previous definition on line %ld as a %s.\n",
                       fdef->function_def.identifier, fdef->line, symbol->line,
                       symbol_type_humanstr(symbol->symbol.symtype));
                return false;
        }

        astnode_compound_foreach(fdef->function_def.params->block.nodes, fdef, (void *) declare_param_variable);

        if (!analyze_any(sem, fdef->function_def.block))
                return false;

        put_symbol(fdef->super,
                   astnode_symbol(fdef->super, SYMBOL_FUNCTION, fdef->function_def.identifier, fdef->function_def.type,
                                  fdef));

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