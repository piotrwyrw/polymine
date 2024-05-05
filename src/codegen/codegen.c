
#include "codegen.h"
#include "../semantics/semutil.h"

#define EMIT(...) fprintf(gen->out, __VA_ARGS__)
#define EMITB(...) EMIT(__VA_ARGS__); break

#define _codegen struct codegen *gen

void codegen_init(struct codegen *codegen, struct astnode *program, struct astnode *stuff, FILE *out)
{
        codegen->program = program;
        codegen->stuff = stuff;
        codegen->out = out;
}

static void *gen_lambdas(_codegen, struct astnode *node)
{
        if (node->type != NODE_GENERATED_FUNCTION)
                return NULL;

        if (node->generated_function.type != GENERATED_LAMBDA)
                return NULL;

        gen_any(gen, node);
        return NULL;
}

static void *gen_includes(_codegen, struct astnode *node)
{
        if (node->type != NODE_INCLUDE)
                return NULL;

        gen_any(gen, node);
        return NULL;
}

static void gen_bootstrap(_codegen)
{
        EMIT("void __main();\n"
             "\n"
             "int main(void) {\n"
             "        __main();\n"
             "        return 0;\n"
             "}\n\n");
}

void gen_generate(struct codegen *gen)
{
        printf("Generating C code ..\n");

        // First generate the includes ..
        astnode_compound_foreach(gen->stuff, gen, (void *) gen_includes);

        EMIT("\n");

        // .. then the bootstrapping code ..
        gen_bootstrap(gen);

        // .. then the lambdas ..
        astnode_compound_foreach(gen->stuff, gen, (void *) gen_lambdas);

        // .. and then, finally, move on to the actual program code
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
        switch (node->type) {
                case NODE_PROGRAM:
                        gen_compound(gen, node->program.block->block.nodes);
                        break;
                case NODE_GENERATED_FUNCTION:
                case NODE_FUNCTION_DEFINITION:
                        gen_function_definition(gen, node);
                        break;
                case NODE_INCLUDE:
                        gen_include(gen, node);
                        break;
                default:
                        break;
        }
}

void gen_include(_codegen, struct astnode *node)
{
        EMIT("#include <%s>\n", node->include.path);
}

void gen_type(_codegen, struct astdtype *type, char *identifier)
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
                                case BUILTIN_UNDEFINED:
                                        break; // Shouldn't happen
                        }
                        break;
                case ASTDTYPE_POINTER:
                        gen_type(gen, type->pointer.to, NULL);
                        EMITB("*");
                case ASTDTYPE_LAMBDA:
                        break; // TODO
                case ASTDTYPE_CUSTOM:
                EMITB("%s", type->custom.name);
        }
}

void gen_function_definition(_codegen, struct astnode *_fdef)
{
        struct astnode *fdef;

        if (_fdef->type == NODE_FUNCTION_DEFINITION)
                fdef = _fdef;
        else
                fdef = _fdef->generated_function.definition;

        gen_type(gen, fdef->function_def.type, fdef->function_def.generated->generated_function.generated_id);
        EMIT(" %s()\n{\n}\n\n", fdef->function_def.generated->generated_function.generated_id);
}

#undef EMITB
#undef EMIT
#undef _codegen