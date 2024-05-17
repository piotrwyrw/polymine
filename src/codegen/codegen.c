
#include "codegen.h"
#include "../semantics/semutil.h"

#define EMIT(...) fprintf(gen->out, __VA_ARGS__)
#define EMITB(...) EMIT(__VA_ARGS__); break

#define _codegen struct codegen *gen

void codegen_init(struct codegen *codegen, struct astnode *program, struct astnode *stuff, size_t counter, FILE *out)
{
        codegen->program = program;
        codegen->stuff = stuff;
        codegen->out = out;
        codegen->symbol_counter = counter;
        codegen->param_count = 0;
        codegen->param_no = 0;
}

static void *gen_includes(_codegen, struct astnode *node)
{
        if (node->type != NODE_INCLUDE)
                return NULL;

        gen_include(gen, node);
        return NULL;
}

static void gen_bootstrap(_codegen)
{
        EMIT("void pFn_polymine_bootstrap();\n"
             "\n"
             "int main(void) {\n"
             "        pFn_polymine_bootstrap();\n"
             "        return 0;\n"
             "}\n\n");
}

void gen_generate(_codegen)
{
        printf("Generating C code ..\n");

        // First generate the includes …
        astnode_compound_foreach(gen->stuff, gen, (void *) gen_includes);

        EMIT("\n");

        // … then the bootstrapping code …
        gen_bootstrap(gen);

        // … and then, finally, move on to the actual program code
        gen_any(gen, gen->program);

        printf("Code generation done!\n");
}

static void gen_compound(_codegen, struct astnode *compound)
{
        for (size_t i = 0; i < compound->node_compound.count; i++)
                gen_any(gen, compound->node_compound.array[i]);
}

void gen_any(_codegen, struct astnode *node)
{
        if (node->ignore)
                return;

        switch (node->type) {
                case NODE_PROGRAM:
                        gen_compound(gen, node->program.block->block.nodes);
                        break;
                case NODE_BLOCK:
                        gen_compound(gen, node->block.nodes);
                        break;
                case NODE_COMPOUND:
                        gen_compound(gen, node);
                        break;
                case NODE_GENERATED_FUNCTION:
                case NODE_FUNCTION_DEFINITION:
                        gen_function_definition(gen, node);
                        break;
                case NODE_VARIABLE_DECL:
                        gen_variable_declaration(gen, node);
                        break;
                case NODE_INCLUDE:
                        break;
                case NODE_RESOLVE:
                        gen_resolve(gen, node);
                        break;
                case NODE_FUNCTION_CALL:
                case NODE_BINARY_OP:
                case NODE_STRING_LITERAL:
                case NODE_FLOAT_LITERAL:
                case NODE_INTEGER_LITERAL:
                case NODE_PATH:
                case NODE_VARIABLE_USE:
                case NODE_POINTER:
                case NODE_DEREFERENCE:
                        gen_expression(gen, node);
                        if (!node->holder)
                                EMIT(";\n");
                        break;
                case NODE_PRESENT_FUNCTION:
                        break;
                case NODE_IF:
                        gen_if(gen, node, 0);
                        break;
                case NODE_VARIABLE_ASSIGNMENT:
                        gen_assignment(gen, node);
                        break;
                case NODE_COMPLEX_TYPE:
                        gen_type_definition(gen, node);
                case NODE_NOTHING:
                        break;
                default:
                        printf("Unknown node type passed to gen_any(..): %s\n", nodetype_string(node->type));
                        break;
        }
}

void gen_resolve(_codegen, struct astnode *node)
{
        if (node->resolve.value->type != NODE_VOID_PLACEHOLDER) {
                struct astnode *resolveValueVariable = temporary_variable(gen,
                                                                          node->resolve.function->function_def.type,
                                                                          WRAP(node->resolve.value));
                gen_variable_declaration(gen, resolveValueVariable);

                EMIT("return %s;\n", resolveValueVariable->declaration.generated_id);
                return;
        }

        EMIT("return ");
        gen_expression(gen, node->resolve.value);
        EMIT(";\n");
}

void gen_if(_codegen, struct astnode *node, size_t branch_number)
{
        struct astnode *ifExprVar = NULL;

        if (node->if_statement.expr) {
                ifExprVar = temporary_variable(gen, node->if_statement.exprType, WRAP(node->if_statement.expr));
                gen_variable_declaration(gen, ifExprVar);
        }

        if (branch_number == 0)
                EMIT("if");
        else {
                EMIT("else");
                if (node->if_statement.expr)
                        EMIT(" if");
                else
                        EMIT(" ");
        }

        if (ifExprVar)
                EMIT(" (%s)", ifExprVar->declaration.generated_id);

        EMIT("{\n");

        gen_any(gen, node->if_statement.block);

        EMIT("}\n");

        if (node->if_statement.next_branch)
                gen_if(gen, node->if_statement.next_branch, branch_number + 1);
}

void gen_include(_codegen, struct astnode *node)
{
        EMIT("#include <%s>\n", node->include.path);
}

static void *gen_param(_codegen, struct astnode *_param)
{
        struct astnode *param = UNWRAP(_param);

        gen_type(gen, param->declaration.type);

        EMIT(" %s", param->declaration.generated_id);

        if (gen->param_no + 1 < gen->param_count)
                EMIT(", ");

        gen->param_no++;

        return NULL;
}

void gen_type(_codegen, struct astdtype *type)
{
        switch (type->type) {
                case ASTDTYPE_VOID:
                EMITB("void");
                case ASTDTYPE_BUILTIN:
                        switch (type->builtin.datatype) {
                                case BUILTIN_GENERIC_BYTE:
                                case BUILTIN_CHAR:
                                EMITB("char");
                                case BUILTIN_DOUBLE:
                                EMITB("double");
                                case BUILTIN_INT8:
                                EMITB("int8_t");
                                case BUILTIN_INT16:
                                EMITB("int16_t");
                                case BUILTIN_INT32:
                                EMITB("int32_t");
                                case BUILTIN_INT64:
                                EMITB("int64_t");
                                case BUILTIN_STRING:
                                EMITB("char *");
                                case BUILTIN_UNDEFINED:
                                        break; // Shouldn't happen
                        }
                        break;
                case ASTDTYPE_POINTER:
                        gen_type(gen, type->pointer.to);
                        EMITB("*");
                case ASTDTYPE_COMPLEX:
                EMITB("struct %s", type->complex.definition->type_definition.generated_identifier);
        }
}

static void *gen_call_param(_codegen, struct astnode *);

// TODO
void gen_path(_codegen, struct astnode *node, struct astnode *until)
{
        struct astnode *segment = node;
        struct astnode *lastCallSegment = last_call_path(node, until);

        // If there are no function calls in the path (OR the function
        // is on the first place), we can just generate it as we usually would
        if (!lastCallSegment || (lastCallSegment && lastCallSegment == segment)) {
                while (segment && segment != until) { // Yes, we're comparing pointers here (and above), and it's fine!
                        gen_expression(gen, segment->path.expr);
                        segment = segment->path.next;
                        if (segment && segment != until)
                                EMIT(".");
                }
                return;
        }

        // Otherwise we need to get a little more fancy ...
        EMIT("%s(",
             lastCallSegment->path.expr->function_call.definition->function_def.generated->generated_function.generated_id);

        struct astnode *fdef = lastCallSegment->path.expr->function_call.definition;

        // First, generate the function parameters as usual
        size_t oldCount = gen->param_count;
        size_t oldNo = gen->param_no;

        size_t actual_param_count = fdef->function_def.param_count - fdef->function_def.provided_param_count;

        gen->param_count = fdef->function_def.param_count;
        gen->param_no = 0;

        for (size_t i = 0; i < actual_param_count; i++)
                gen_call_param(gen, lastCallSegment->path.expr->function_call.values->node_compound.array[i]);

        gen->param_count = oldCount;
        gen->param_no = oldNo;

        gen_path(gen, node, lastCallSegment);
        EMIT(")");

        if (lastCallSegment->path.next && lastCallSegment->path.next->path.expr->type != NODE_FUNCTION_CALL) {
                EMIT(".");
                gen_path(gen, lastCallSegment->path.next, NULL);
        }
}

static void *gen_complex_field(_codegen, struct astnode *field)
{
        gen_type(gen, field->declaration.type);
        EMIT(" %s;\n", field->declaration.generated_id);
        return NULL;
}

static void *gen_complex_function(_codegen, struct astnode *fdef)
{
        gen_function_definition(gen, fdef);
        return NULL;
}

void gen_type_definition(_codegen, struct astnode *def)
{
        EMIT("struct %s {\n", def->type_definition.generated_identifier);

        astnode_compound_foreach(def->type_definition.fields, gen, (void *) gen_complex_field);

        EMIT("};\n");

        astnode_compound_foreach(def->type_definition.block->block.nodes, gen, (void *) gen_complex_function);
}

void gen_function_definition(_codegen, struct astnode *_fdef)
{
        struct astnode *fdef;

        if (_fdef->type == NODE_FUNCTION_DEFINITION)
                fdef = _fdef;
        else
                fdef = _fdef->generated_function.definition;

        gen_type(gen, fdef->function_def.type);
        EMIT(" %s(", fdef->function_def.generated->generated_function.generated_id);

        gen->param_count = fdef->function_def.params->node_compound.count;
        gen->param_no = 0;

        astnode_compound_foreach(fdef->function_def.params, gen, (void *) gen_param);

        EMIT(")\n{\n");
        gen_any(gen, fdef->function_def.block);
        EMIT("}\n\n");
}

static void *gen_default_initializer(_codegen, struct astnode *field)
{
        if (!field->declaration.value)
                return NULL;

        EMIT(".%s = ", field->declaration.generated_id);

        gen_expression(gen, field->declaration.value);

        if (gen->param_no + 1 < gen->param_count)
                EMIT(", ");

        gen->param_no++;

        return NULL;
}

void gen_variable_declaration(_codegen, struct astnode *decl)
{
        struct astnode *value = UNWRAP(decl->declaration.value);

        gen_type(gen, decl->declaration.type);
        EMIT(" %s", decl->declaration.generated_id);
        if (value) {
                EMIT(" = ");
                gen_expression(gen, value);
        } else if (decl->declaration.type->type == ASTDTYPE_COMPLEX) {
                EMIT(" = {");

                struct astnode *typeDef = decl->declaration.type->complex.definition;
                struct astnode *fields = typeDef->type_definition.fields;
                size_t n = 0;

                // Find out how many fields have a default initializer
                for (size_t i = 0; i < fields->node_compound.count; i++)
                        if (fields->node_compound.array[i]->declaration.value)
                                n++;

                size_t oldCount = gen->param_count;
                size_t oldNo = gen->param_no;

                gen->param_count = n;
                gen->param_no = 0;

                astnode_compound_foreach(typeDef->type_definition.fields, gen,
                                         (void *) gen_default_initializer);

                gen->param_count = oldCount;
                gen->param_no = oldNo;

                EMIT("}");
        }
        EMIT(";\n");
}

void gen_assignment(_codegen, struct astnode *assignment)
{
        gen_expression(gen, assignment->assignment.path);
        EMIT(" = ");
        gen_expression(gen, assignment->assignment.value);
        EMIT(";\n");
}

static void *gen_call_param(_codegen, struct astnode *expr)
{
        gen_expression(gen, expr);

        if (gen->param_no + 1 < gen->param_count)
                EMIT(", ");

        gen->param_no++;

        return NULL;
}

void gen_expression(_codegen, struct astnode *_expr)
{
        struct astnode *expr = UNWRAP(_expr);

        struct astnode *n;
        switch (expr->type) {
                case NODE_INTEGER_LITERAL:
                EMITB("%lld", expr->integer_literal.integerValue);
                case NODE_FLOAT_LITERAL:
                EMITB("%f", expr->float_literal.floatValue);
                case NODE_STRING_LITERAL:
                EMITB("\"%s\"", expr->string_literal.value);
                case NODE_FUNCTION_DEFINITION:
                EMITB("%s", expr->function_def.generated->generated_function.generated_id);
                case NODE_VARIABLE_USE:
                EMITB("%s", expr->variable.var->declaration.generated_id);
                case NODE_BINARY_OP:
                        gen_expression(gen, expr->binary.left);
                        EMIT(" %s ", binaryop_cstr(expr->binary.op));
                        gen_expression(gen, expr->binary.right);
                        break;
                case NODE_FUNCTION_CALL:
                        n = expr->function_call.definition;

                        char *id = (n->type == NODE_FUNCTION_DEFINITION)
                                   ? expr->function_call.definition->function_def.generated->generated_function.generated_id
                                   : expr->function_call.identifier;

                        EMIT("%s(", id);

                        size_t oldParamCount = gen->param_count;
                        size_t oldParamNo = gen->param_no;

                        gen->param_count = expr->function_call.values->node_compound.count;
                        gen->param_no = 0;

                        astnode_compound_foreach(expr->function_call.values, gen, (void *) gen_call_param);

                        gen->param_count = oldParamCount;
                        gen->param_no = oldParamNo;

                        EMIT(")");
                        break;
                case NODE_POINTER:
                        EMIT("&(");
                        gen_expression(gen, expr->pointer.target);
                        EMITB(")");
                case NODE_DEREFERENCE:
                        EMIT("(*");
                        gen_expression(gen, expr->dereference.target);
                        EMITB(")");
                case NODE_VOID_PLACEHOLDER:
                EMITB("/* void */");
                case NODE_PATH:
                        gen_path(gen, expr, NULL);
                        break;
                default:
                        break;
        }
}

struct astnode *temporary_variable(_codegen, struct astdtype *type, struct astnode *value)
{
        struct astnode *var = astnode_declaration(0, NULL, true, "", type, value);
        declaration_generate_name(var, gen->symbol_counter++);
        register_allocation(gen, var);
        return var;
}

void register_allocation(_codegen, struct astnode *node)
{
        astnode_push_compound(gen->stuff, node);
}

#undef EMITB
#undef EMIT
#undef _codegen