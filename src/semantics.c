#include "semantics.h"
#include "semutil.h"

#include <stdbool.h>
#include <string.h>

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
                case NODE_FUNCTION_CALL:
                case NODE_POINTER:
                        return analyze_expression(sem, node) != NULL;
                case NODE_VARIABLE_ASSIGNMENT:
                        return analyze_assignment(sem, node);
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
                printf("Type evaluation failed for variable \"%s\" on line %ld.\n", decl->declaration.identifier,
                       decl->line);
                return false;
        }

        if (!types_compatible(decl->declaration.type, exprType)) {
                char *exprTypeStr = strdup(astdtype_string(exprType));
                char *declTypeStr = strdup(astdtype_string(decl->declaration.type));
                printf("The effective type of the expression (%s) is not compatible with the variable declaration type (%s) on line %ld.\n",
                       exprTypeStr, declTypeStr,
                       decl->line);
                free(exprTypeStr);
                free(declTypeStr);
                return false;
        }

        put_and_exit:
        put_symbol(decl->super,
                   astnode_symbol(decl->super, SYMBOL_VARIABLE, decl->declaration.identifier, decl->declaration.type,
                                  decl));

        return true;
}

_Bool analyze_assignment(struct semantics *sem, struct astnode *assignment)
{
        struct astnode *sym;

        if (!(sym = find_symbol(assignment->assignment.identifier, assignment->super))) {
                printf("Attempted to assign undefined variable \"%s\" on line %ld.\n",
                       assignment->assignment.identifier, assignment->line);
                return false;
        }

        if (sym->symbol.symtype != SYMBOL_VARIABLE) {
                printf("The symbol \"%s\" is a %s. Attempted assignment in variable context on line %ld.\n",
                       assignment->assignment.identifier,
                       symbol_type_humanstr(sym->symbol.symtype), assignment->line);
                return false;
        }

        struct astdtype *exprType = analyze_expression(sem, assignment->assignment.value);

        if (!exprType) {
                printf("Type validation of assignment expression failed on line %ld.\n", assignment->line);
                return false;
        }

        struct astdtype *varType = sym->symbol.type;

        if (!types_compatible(varType, exprType)) {
                char *exprTypeStr = strdup(astdtype_string(exprType));
                char *varTypeStr = strdup(astdtype_string(varType));

                printf("Cannot assign \"%s\" (%s) to an expression of type %s. Type compatibility error on line %ld.\n",
                       assignment->assignment.identifier, varTypeStr, exprTypeStr);

                free(exprTypeStr);
                free(varTypeStr);

                return false;
        }

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
                case NODE_FUNCTION_CALL:
                case NODE_POINTER:
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
        if (atom->type == NODE_INTEGER_LITERAL) {
                struct astdtype *type = required_type_integer(sem, atom->integer_literal.integerValue);
                if (!type) {
                        printf("Could not find appropriate builtin type for the provided integer literal on line %ld.\n",
                               atom->line);
                        return NULL;
                }
                return type;
        }

        if (atom->type == NODE_FLOAT_LITERAL)
                return sem->_double;

        if (atom->type == NODE_STRING_LITERAL) {
                if (atom->holder->type == NODE_BINARY_OP) {
                        printf("A string literal cannot be part of a binary expression. Violation (\"%s\") on line %ld.\n",
                               atom->string_literal.value, atom->line);
                        return NULL;
                }
                return sem->int64;
        }

        if (atom->type == NODE_FUNCTION_CALL)
                return analyze_function_call(sem, atom);

        if (atom->type == NODE_POINTER) {
                struct astdtype *exprType = analyze_expression(sem, atom->pointer.target);

                if (!exprType) {
                        printf("Could not create pointer on line %ld: Type checking failed.\n", atom->line);
                        return NULL;
                }

                return semantics_newtype(sem, astdtype_pointer(exprType));
        }

        return NULL;
}

struct astdtype *analyze_function_call(struct semantics *sem, struct astnode *call)
{
        struct astnode *symbol;

        if (!(symbol = find_symbol(call->function_call.identifier, call->super))) {
                printf("Attempting to call undefined function \"%s\". Error on line %ld.\n",
                       call->function_call.identifier, call->line);
                return NULL;
        }

        if (symbol->symbol.symtype != SYMBOL_FUNCTION) {
                printf("Attempting to call %s as function. Error on line %ld.\n",
                       symbol_type_humanstr(symbol->symbol.symtype), call->line);
                return NULL;
        }

        struct astnode *definition = symbol->symbol.node;

        size_t required_params = definition->function_def.params->block.nodes->node_compound.count;
        size_t provided_params = call->function_call.values->block.nodes->node_compound.count;

        if (required_params != provided_params) {
                printf("The function \"%s\" (%s) expects %ld params. %ld Parameters were provided in the call. Error on line %ld\n",
                       call->function_call.identifier, astdtype_string(definition->function_def.type), required_params,
                       provided_params, call->line);
                return NULL;
        }

        for (size_t i = 0; i < call->function_call.values->block.nodes->node_compound.count; i++) {
                struct astnode *callValue = call->function_call.values->block.nodes->node_compound.array[i];
                struct astdtype *valueType = analyze_expression(sem, callValue);
                struct astnode *param = definition->function_def.params->block.nodes->node_compound.array[i];
                struct astdtype *reqParamType = param->declaration.type;

                if (!types_compatible(reqParamType, valueType)) {
                        printf("The expression type is not compatible with the parameter number %ld \"%s\" of function \"%s\". Error on line %ld.\n",
                               i + 1, param->declaration.identifier, definition->function_def.identifier, call->line);
                        return NULL;
                }
        }

        return definition->function_def.type;
}