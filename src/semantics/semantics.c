#include "semantics.h"
#include "semutil.h"

#include <stdbool.h>
#include <string.h>

_Bool analyze_program(struct semantics *sem, struct astnode *program)
{
        if (!analyze_any(sem, program->program.block))
                return false;

        struct astnode *sym;

        if (!(sym = find_symbol("main", program->program.block))) {
                printf("A void-typed main function could not be found.\n");
                return false;
        }

        if (sym->symbol.symtype != SYMBOL_FUNCTION) {
                printf("The symbol 'main' is a %s. Expected function. Conflict on line %ld.\n",
                       symbol_type_humanstr(sym->symbol.symtype), sym->symbol.node->line);
                return false;
        }

        struct astdtype *type = sym->symbol.node->function_def.type;

        if (type->type != ASTDTYPE_VOID) {
                printf("Expected the main function to be of type void. Error on line %ld\n", sym->symbol.node->line);
                return false;
        }

        if (sym->symbol.node->function_def.params->node_compound.count != 0) {
                printf("Expected the main function to have no parameters. Error on line %ld.\n",
                       sym->symbol.node->line);
                return false;
        }

        return true;
}

_Bool analyze_compound(struct semantics *sem, struct astnode *compound)
{
        _Bool success = true;

        for (size_t i = 0; i < compound->node_compound.count; i++)
                if (!analyze_any(sem, compound->node_compound.array[i]))
                        success = false;

        return success;
}

_Bool analyze_block(struct semantics *sem, struct astnode *block)
{
        if (!block->holder) {
                printf("Dangling blocks are not allowed. Please use scopes to control variable visibility.\n");
                return false;
        }

        return analyze_compound(sem, block->block.nodes);
}

_Bool analyze_any(struct semantics *sem, struct astnode *node)
{
        if (node->type != NODE_INCLUDE &&
            !(node->type == NODE_BLOCK && node->holder && node->holder->type == NODE_PROGRAM))
                sem->pristine = false;

        switch (node->type) {
                case NODE_BLOCK:
                        return analyze_block(sem, node);
                case NODE_IF:
                        return analyze_if(sem, node);
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
                case NODE_DEREFERENCE:
                        return analyze_expression(sem, node, NULL, NULL) != NULL;
                case NODE_VARIABLE_ASSIGNMENT:
                        return analyze_assignment(sem, node);
                case NODE_RESOLVE:
                        return analyze_resolve(sem, node);
                case NODE_INCLUDE:
                        return analyze_include(sem, node);
                case NODE_NOTHING:
                        return true;
                case NODE_PRESENT_FUNCTION:
                        return analyze_present_function(sem, node);
                case NODE_COMPLEX_TYPE:
                        return analyze_complex_type(sem, node);
                case NODE_PATH:
                        return analyze_path(sem, node) != NULL;
                default:
                        printf("Unknown node type passed to analyze_any(..): %s\n", nodetype_string(node->type));
                        return false;
        }
}

_Bool analyze_if(struct semantics *sem, struct astnode *_if)
{
        if (!_if->if_statement.expr)
                goto skip_expression;

        struct astdtype *type = analyze_expression(sem, _if->if_statement.expr, NULL, NULL);

        if (!type) {
                printf("Type evaluation failed for if condition on line %ld.\n", _if->line);
                return false;
        }

        if (type->type != ASTDTYPE_BUILTIN) {
                printf("The if-expression on line %ld is invalid: The expression must evaluate to an effective builtin type.\n",
                       _if->line);
                return false;
        }

        skip_expression:

        if (!analyze_block(sem, _if->if_statement.block))
                return false;

        if (_if->if_statement.next_branch)
                return analyze_if(sem, _if->if_statement.next_branch);

        return true;
}

_Bool analyze_type(struct semantics *sem, struct astdtype *type, struct astnode *consumer)
{
        if (type->type != ASTDTYPE_COMPLEX && type->type != ASTDTYPE_POINTER)
                return true;

        if (type->type == ASTDTYPE_POINTER)
                return analyze_type(sem, type->pointer.to, consumer);

        struct astnode *sym = find_symbol(type->complex.name, consumer->super);

        if (!sym || (sym && sym->symbol.symtype != SYMBOL_TYPEDEF)) {
                printf("The complex type \"%s\" does not exist. Error on line %ld.\n", type->complex.name,
                       consumer->line);
                return false;
        }

        type->complex.definition = sym->symbol.node;

        return true;
}

_Bool analyze_variable_declaration(struct semantics *sem, struct astnode *decl)
{
        if (symbol_conflict(decl->declaration.identifier, decl))
                return false;

        if (decl->declaration.type && !analyze_type(sem, decl->declaration.type, decl)) {
                printf("The type of variable \"%s\" is invalid. Error on line %ld.\n", decl->declaration.identifier,
                       decl->line);
                return false;
        }

        // Type-inferred variables must have a value at the time of declaration
        if (!decl->declaration.type && !decl->declaration.value) {
                printf("Type-inferred variables rely on a required initial expression to infer their types. Violation on line %ld.\n",
                       decl->line);
                return false;
        }

        if (decl->declaration.type && decl->declaration.type->type == ASTDTYPE_VOID) {
                printf("Void is not a valid type for a variable. Error on line %ld.\n", decl->line);
                return false;
        }

        if (!decl->declaration.value)
                goto put_and_exit;

        _Bool compile_time;

        struct astdtype *exprType = analyze_expression(sem, UNWRAP(decl->declaration.value), &compile_time, NULL);

        if (!exprType) {
                printf("Type evaluation failed for variable \"%s\" on line %ld.\n", decl->declaration.identifier,
                       decl->line);
                return false;
        }

        if (exprType->type == ASTDTYPE_VOID) {
                printf("A variable may not be assigned to a void type. Violation on line %ld.\n", decl->line);
                return false;
        }

        _Bool uppermost = is_uppermost_block(decl->super);

        if (!compile_time && uppermost) {
                printf("The value of variable \"%s\" is not strictly a compile-time constant and thus may not be used as the initial value of a global variable.\n",
                       decl->declaration.identifier);
                return false;
        }

        // Type inference
        if (!decl->declaration.type) {
                decl->declaration.type = exprType;
                goto put_and_exit;
        }

        if (!types_compatible(decl->declaration.type, exprType)) {
                char *exprTypeStr = astdtype_string(exprType);
                char *declTypeStr = astdtype_string(decl->declaration.type);

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

        declaration_generate_name(decl, sem->symbol_counter++);

        return true;
}

_Bool analyze_assignment(struct semantics *sem, struct astnode *assignment)
{
        struct astnode *target = analyze_path(sem, assignment->assignment.path);

        if (!target) {
                printf("Assignment path is invalid. Error on line %ld.\n", assignment->line);
                return false;
        }

        if (target->type != NODE_VARIABLE_USE) {
                printf("Attempted to assign %s as variable. Error on line %ld.\n", nodetype_string(assignment->type),
                       assignment->line);
                return false;
        }

        target = target->variable.var;

        assignment->assignment.declaration = target;

        struct astdtype *exprType = analyze_expression(sem, assignment->assignment.value, NULL, NULL);

        if (!exprType) {
                printf("Type validation of assignment expression failed on line %ld.\n", assignment->line);
                return false;
        }

        if (exprType->type == ASTDTYPE_VOID) {
                printf("A variable may not be assigned to a void type. Violation on line %ld.\n", assignment->line);
                return false;
        }

        struct astdtype *varType = target->declaration.type;

        if (!types_compatible(varType, exprType)) {
                char *exprTypeStr = astdtype_string(exprType);
                char *varTypeStr = astdtype_string(varType);

                printf("Cannot assign \"%s\" (%s) to an expression of type %s. Type compatibility error on line %ld.\n",
                       target->declaration.identifier, varTypeStr, exprTypeStr, assignment->line);

                free(exprTypeStr);
                free(varTypeStr);

                return false;
        }

        return true;
}

static struct semantics *_semantics;

static void *declare_param_variable(struct astnode *fdef, struct astnode *variable)
{
        if (!analyze_type(_semantics, variable->declaration.type, variable))
                return variable;

        declaration_generate_name(variable, _semantics->symbol_counter++);

        put_symbol(fdef->function_def.block,
                   astnode_symbol(fdef->function_def.block, SYMBOL_VARIABLE, variable->declaration.identifier,
                                  variable->declaration.type, variable));
        return NULL;
}

static void *analyze_linked_function_params(struct semantics *sem, struct astnode *param)
{
        if (!analyze_type(sem, param->declaration.type, param))
                return param;

        return NULL;
}

_Bool analyze_present_function(struct semantics *sem, struct astnode *present)
{
        if (symbol_conflict(present->present_function.identifier, present))
                return false;

        if (present->super->super) {
                printf("A present function definition must be placed at the root of the program. Function \"%s\" violated this rule on line %ld.\n",
                       present->present_function.identifier, present->line);
                return false;
        }

        struct astnode *flawed_param;

        if ((flawed_param = astnode_compound_foreach(present->present_function.params, sem,
                                                     (void *) analyze_linked_function_params))) {
                printf("The type of parameter \"%s\" of function \"%s\" is invalid. Error on line %ld.\n",
                       flawed_param->declaration.identifier,
                       present->present_function.identifier, present->line);
                return false;
        }

        put_symbol(present->super, astnode_symbol(present->super, SYMBOL_FUNCTION, present->present_function.identifier,
                                                  present->present_function.type, present));

        return true;
}

_Bool analyze_function_definition(struct semantics *sem, struct astnode *fdef)
{
        if (symbol_conflict(fdef->function_def.identifier, fdef))
                return false;

        if (!analyze_type(sem, fdef->function_def.type, fdef)) {
                printf("Type analysis failed for return type of \"%s\". Error on line %ld.\n",
                       fdef->function_def.identifier, fdef->line);
                return false;
        }

        if (fdef->super && fdef->super->holder && fdef->super->holder->type != NODE_PROGRAM) {
                printf("Functions must be declared in the global scope. Error on line %ld.\n", fdef->line);
                return false;
        }

        struct astnode *flawed_param;

        _semantics = sem;

        if ((flawed_param = astnode_compound_foreach(fdef->function_def.params, fdef,
                                                     (void *) declare_param_variable))) {
                printf("The provided type for parameter variable \"%s\" of function \"%s\" is invalid. Error on line %ld.\n",
                       flawed_param->declaration.identifier, fdef->function_def.identifier, flawed_param->line);
                return false;
        }

        _semantics = NULL;

        struct astnode *sym;

        put_symbol(fdef->function_def.block,
                   sym = astnode_symbol(fdef->super, SYMBOL_FUNCTION, fdef->function_def.identifier,
                                        fdef->function_def.type,
                                        fdef));

        if (!analyze_any(sem, fdef->function_def.block))
                return false;


        if (has_attribute(fdef->function_def.attributes, "no_return_checks"))
                goto skip_return_checks;

        if (!fdef->function_def.conditionless_resolve && fdef->function_def.type->type != ASTDTYPE_VOID) {
                printf("The non-void function \"%s\" declared on line %ld does not have an always-reachable resolve statement.\n",
                       FUNCTION_ID(fdef->function_def.identifier), fdef->line);
                return false;
        }

        skip_return_checks:

        // It's important to make the function available in the global scope only after analyzing the function block
        if (fdef->function_def.identifier)
                put_symbol(fdef->super, astnode_copy_symbol(sym));

        semantics_new_function(sem, fdef);

        fdef->function_def.param_count = fdef->function_def.params->node_compound.count;

        return true;
}

void *analyze_complex_type_field(struct semantics *sem, struct astnode *field)
{
        struct astdtype *type = field->declaration.type;

        if (!analyze_type(sem, type, field))
                return field;

        if (field->declaration.value) {
                _Bool compileTime;
                struct astdtype *exprType = analyze_expression(sem, field->declaration.value, &compileTime, NULL);

                if (!exprType) {
                        printf("Type analysis failed for default expression of field \"%s\" on line %ld.\n",
                               field->declaration.identifier, field->line);
                        return field;
                }

                if (!compileTime) {
                        printf("The default value for field \"%s\" is not a compile-time constant. Error on line %ld.\n",
                               field->declaration.identifier, field->line);
                        return field;
                }

                if (!types_compatible(type, exprType)) {
                        printf("The type of field \"%s\" is not compatible with the assigned default value. Error on line %ld.\n",
                               field->declaration.identifier, field->line);
                        return field;
                }
        }

        declaration_generate_name(field, sem->symbol_counter++);
        return NULL;
}

_Bool analyze_complex_type(struct semantics *sem, struct astnode *def)
{
        if (astnode_compound_foreach(def->type_definition.fields, sem, (void *) analyze_complex_type_field)) {
                printf("Type analysis failed for fields of type \"%s\". Error on line %ld.\n",
                       def->type_definition.identifier, def->line);
                return false;
        }

        complex_type_generate_name(def, sem->symbol_counter++);

        struct astdtype *type = astdtype_complex(def->type_definition.identifier);
        type->complex.definition = def;

        semantics_new_type(sem, type);

        put_symbol(def->super, astnode_symbol(def->super, SYMBOL_TYPEDEF, def->type_definition.identifier, type, def));

        return true;
}

_Bool analyze_resolve(struct semantics *sem, struct astnode *res)
{
        struct astdtype *type = analyze_expression(sem, res->resolve.value, NULL, NULL);
        struct astnode *function;

        if (!type)
                return false;

        if (!(function = find_enclosing_function(res->super))) {
                printf("A resolve statement may only be placed inside of a function. Violation on line %ld.\n",
                       res->line);
                return false;
        }

        if (!types_compatible(function->function_def.type, type)) {
                char *exprType = astdtype_string(type);
                char *fnType = astdtype_string(function->function_def.type);

                printf("The resolve statement expression type (%s) is not compatible with the function type (%s) on line %ld.\n",
                       exprType, fnType, res->line);

                free(exprType);
                free(fnType);

                return false;
        }

        if (!find_uncertain_reachability_structures(res->super))
                function->function_def.conditionless_resolve = true;

        res->resolve.function = function;

        return true;
}

_Bool analyze_include(struct semantics *sem, struct astnode *include)
{
        if (!sem->pristine) {
                printf("Include statements must be located at the very top of the file. Error on line %ld.\n",
                       include->line);
                return false;
        }

        semantics_new_include(sem, include->include.path);

        return true;
}

static struct astdtype *analyze_path_as_expression(struct semantics *, struct astnode *);

struct astdtype *analyze_expression(struct semantics *sem, struct astnode *expr, _Bool *compile_time, struct astnode *def)
{
        // The default state is true. This might change during the recursive call chain
        if (compile_time)
                *compile_time = true;

        switch (expr->type) {
                case NODE_BINARY_OP:
                        return analyze_binary_expression(sem, expr, compile_time, def);
                case NODE_VARIABLE_USE:
                        return analyze_variable_use(sem, expr, def);
                case NODE_INTEGER_LITERAL:
                case NODE_FLOAT_LITERAL:
                case NODE_STRING_LITERAL:
                case NODE_FUNCTION_CALL:
                case NODE_POINTER:
                case NODE_DEREFERENCE:
                        return analyze_atom(sem, expr, compile_time, def);
                case NODE_VOID_PLACEHOLDER:
                        return sem->_void;
                case NODE_PATH:
                        return analyze_path_as_expression(sem, expr);
                default:
                        return NULL;
        }
}

struct astdtype *analyze_binary_expression(struct semantics *sem, struct astnode *bin, _Bool *compile_time, struct astnode *def)
{
        struct astdtype *right = analyze_expression(sem, bin->binary.left, compile_time, def);
        struct astdtype *left = analyze_expression(sem, bin->binary.right, compile_time, def);

        if (!left || !right)
                return NULL;

        struct astdtype *type = required_type(left, right);

        if (!type) {
                char *leftType = astdtype_string(left);
                char *rightType = astdtype_string(right);

                printf("Cannot perform binary operation between %s and %s on line %ld.\n", leftType, rightType,
                       bin->line);

                free(leftType);
                free(rightType);

                return NULL;
        }

        if (bin->binary.op == BOP_RGREQ || bin->binary.op == BOP_LGREQ || bin->binary.op == BOP_RGREATER ||
            bin->binary.op == BOP_LGREATER || bin->binary.op == BOP_AND || bin->binary.op == BOP_OR)
                return sem->int8;

        return type;
}

static struct astdtype *analyze_path_as_expression(struct semantics *sem, struct astnode *path)
{
        struct astnode *res = analyze_path(sem, path);

        if (!res)
                return NULL;

        if (res->type == NODE_VARIABLE_USE)
                return res->variable.var->declaration.type;

        if (res->type == NODE_FUNCTION_CALL)
                return res->function_call.definition->function_def.type;

        printf("Could not extract expression type from \"%s\". Error on line %ld.\n", nodetype_string(res->type),
               path->line);

        return NULL;
}

static struct astnode *dereference_all(struct astnode *expr, struct astdtype **type)
{
        struct astnode *deref = expr;
        struct astdtype *outType;
        size_t deg = find_pointer_degree(*type, &outType);

        for (size_t i = 0; i < deg; i++)
                deref = astnode_dereference(expr->line, expr->super, deref);

        *type = outType;

        return deref;
}

struct astnode *analyze_path(struct semantics *sem, struct astnode *path)
{
        struct astnode *pathSegment = path;
        struct astdtype *lastType;
        struct astnode *lastExpr = NULL;

        _Bool first = true;

        while (pathSegment) {
                struct astnode *expr = pathSegment->path.expr;

                // We analyze the first expression as usual
                if (first) {
                        lastExpr = expr;
                        lastType = analyze_expression(sem, expr, NULL, NULL);
                        if (!lastType) {
                                printf("Semantic analysis failed for path on line %ld.\n", path->line);
                                return NULL;
                        }

                        // QoL improvements: Don't require explicit type dereferencing in path
                        if (pathSegment->path.next) {
                                lastExpr = dereference_all(lastExpr, &lastType);
                                pathSegment->path.expr = lastExpr;
                        }

                        first = false;
                        pathSegment->path.target = lastExpr;
                        pathSegment = pathSegment->path.next;
                        continue;
                }

                // The remaining expressions need to be found within the return lastType of the last segment. Thus, we
                // first need to figure out an identifier for the requested field.

                if (lastType->type != ASTDTYPE_COMPLEX) {
                        printf("A builtin lastType may only constitute the terminal pathSegment of a path. Error on line %ld.\n",
                               path->line);
                        return NULL;
                }

                char *id;

                if (expr->type == NODE_VARIABLE_USE)
                        id = expr->variable.identifier;
                else if (expr->type == NODE_FUNCTION_CALL)
                        id = expr->function_call.identifier;
                else {
                        printf("Unexpected pathSegment found when analyzing path expression (%s). Error on line %ld.\n",
                               nodetype_string(expr->type), expr->line);
                        return NULL;
                }

                struct astnode *def = find_in_type(lastType->complex.definition, id);

                lastExpr = expr;

                if (!def) {
                        printf("The complex type \"%s\" does not contain an element named \"%s\". Invalid path. Error on line %ld.\n",
                               lastType->complex.name, id, expr->line);
                        return NULL;
                }

                lastType = analyze_expression(sem, expr, NULL, def);

                if (!lastType) {
                        printf("Failed to analyze path on line %ld.\n", path->line);
                        return NULL;
                }

                // QoL improvements: Don't require explicit type dereferencing in path
                if (pathSegment->path.next) {
                        lastExpr = dereference_all(lastExpr, &lastType);
                        pathSegment->path.expr = lastExpr;
                }

                pathSegment->path.target = lastExpr;

                pathSegment = pathSegment->path.next;
        }

        return lastExpr;
}

struct astdtype *analyze_variable_use(struct semantics *sem, struct astnode *use, struct astnode *def)
{
        struct astnode *symbol;
        struct astnode *node;
        struct astdtype *type;

        if (def) {
                node = def;
                type = def->declaration.type;
                goto _return;
        }

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

        node = symbol->symbol.node;
        type = symbol->symbol.type;

        _return:

        use->variable.var = node;

        return type;
}

struct astdtype *analyze_atom(struct semantics *sem, struct astnode *atom, _Bool *compile_time, struct astnode *def)
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
                if (atom->holder && atom->holder->type == NODE_BINARY_OP) {
                        printf("A string literal cannot be part of a binary expression. Violation (\"%s\") on line %ld.\n",
                               atom->string_literal.value, atom->line);
                        return NULL;
                }
                return sem->string;
        }

        if (atom->type == NODE_FUNCTION_CALL) {
                if (compile_time)
                        *compile_time = false;

                return analyze_function_call(sem, atom, def);
        }

        if (atom->type == NODE_POINTER) {
                struct astnode *target;

                if (atom->pointer.target->type == NODE_PATH) {
                        target = analyze_path(sem, atom->pointer.target);
                        if (!target) {
                                printf("Invalid path on line %ld.\n", atom->line);
                                return NULL;
                        }
                } else target = atom->pointer.target;

                if (target->type != NODE_VARIABLE_USE) {
                        printf("A pointer can only be created to a variable. Error on line %ld.\n", atom->line);
                        return NULL;
                }

                struct astdtype *exprType = analyze_expression(sem, target, compile_time, target->variable.var);

                if (!exprType) {
                        printf("Could not create pointer on line %ld: Type checking failed.\n", atom->line);
                        return NULL;
                }

                if (exprType->type == ASTDTYPE_VOID) {
                        char *typeStr = astdtype_string(exprType);
                        printf("Cannot create pointer to type %s. Error on line %ld.\n", typeStr, atom->line);
                        free(typeStr);
                        return NULL;
                }

                return semantics_new_type(sem, astdtype_pointer(exprType));
        }

        if (atom->type == NODE_DEREFERENCE) {
                struct astnode *target;

                if (atom->dereference.target->type == NODE_PATH) {
                        target = analyze_path(sem, atom->pointer.target);
                        if (!target) {
                                printf("Invalid path on line %ld.\n", atom->line);
                                return NULL;
                        }
                } else target = atom->dereference.target;

                if (target->type != NODE_VARIABLE_USE) {
                        printf("Only a variable may be dereferenced. Error on line %ld.\n", atom->line);
                        return NULL;
                }

                struct astdtype *exprType = analyze_expression(sem, target, compile_time, target->variable.var);

                if (!exprType) {
                        printf("Could not dereference on line %ld: Type checking failed.\n", atom->line);
                        return NULL;
                }

                if (exprType->type != ASTDTYPE_POINTER) {
                        char *typeStr = astdtype_string(exprType);
                        printf("Cannot dereference a non-pointer type %s. Error on line %ld.\n", typeStr, atom->line);
                        free(typeStr);
                        return NULL;
                }

                return exprType->pointer.to;
        }

        return NULL;
}

struct astdtype *analyze_function_call(struct semantics *sem, struct astnode *call, struct astnode *def)
{
        struct astnode *symbol;
        struct astnode *definition;

        if (def) {
                definition = def;
                goto skip_symbol;
        }

        if (!(symbol = find_symbol(call->function_call.identifier, call->super))) {
                printf("Attempting to call undefined function \"%s\". Error on line %ld.\n",
                       FUNCTION_ID(call->function_call.identifier), call->line);
                return NULL;
        }

        if (symbol->symbol.symtype != SYMBOL_FUNCTION) {
                printf("Attempting to call %s \"%s\" as function. Error on line %ld.\n",
                       symbol_type_humanstr(symbol->symbol.symtype), symbol->symbol.identifier, call->line);
                return NULL;
        }

        definition = symbol->symbol.node;

        skip_symbol:;

        size_t required_params;

        if (definition->type == NODE_FUNCTION_DEFINITION)
                required_params = definition->function_def.param_count;
        else
                required_params = definition->present_function.params->node_compound.count;

        size_t provided_params = call->function_call.values->node_compound.count;

        if (required_params != provided_params) {
                char *typeStr = astdtype_string(definition->function_def.type);

                printf("The function \"%s\" (%s) expects %ld params. %ld Parameters were provided in the call. Error on line %ld\n",
                       FUNCTION_ID(call->function_call.identifier), typeStr, required_params,
                       provided_params, call->line);

                free(typeStr);
                return NULL;
        }

        for (size_t i = 0; i < provided_params; i++) {
                struct astnode *callValue = call->function_call.values->node_compound.array[i];

                struct astdtype *valueType = analyze_expression(sem, callValue, NULL, def);
                if (!valueType)
                        return NULL;

                struct astnode *param;

                if (definition->type == NODE_FUNCTION_DEFINITION)
                        param = definition->function_def.params->node_compound.array[i];
                else
                        param = definition->present_function.params->node_compound.array[i];

                struct astdtype *reqParamType = param->declaration.type;

                if (!types_compatible(reqParamType, valueType)) {
                        char *paramType = astdtype_string(reqParamType);
                        char *exprType = astdtype_string(valueType);

                        printf("The expression type \"%s\" is not compatible with the parameter number %ld \"%s\" (%s) of function \"%s\". Error on line %ld.\n",
                               exprType, i + 1, param->declaration.identifier, paramType,
                               FUNCTION_ID(definition->function_def.identifier), call->line);

                        free(paramType);
                        free(exprType);

                        return NULL;
                }
        }

        call->function_call.definition = definition;

        return definition->function_def.type;
}

#undef FUNCTION_ID