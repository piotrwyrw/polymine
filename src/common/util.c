//
// Created by Piotr Krzysztof Wyrwas on 24.03.24.
//

#include "util.h"
#include "../semantics/semantics.h"

#include <string.h>
#include <math.h>

int char_to_digit(char c)
{
        return c - '0';
}

double string_to_float(char *str)
{
        size_t len = strlen(str);
        double output = 0.0;

        size_t dIdx = 0; // Decimal index

        char c;

        for (size_t i = 0; i < len; i++) {
                c = str[i];

                if (c == '.') {
                        dIdx = 1;
                        continue;
                }

                if (dIdx == 0)
                        output = output * 10 + (double) char_to_digit(c);
                else
                        output = output + ((double) char_to_digit(c) / pow(10, (double) dIdx++));
        }

        return output;
}

int64_t string_to_integer(char *str)
{
        size_t len = strlen(str);
        int64_t output = 0;

        for (size_t i = 0; i < len; i++)
                output = output * 10 + char_to_digit(str[i]);

        return output;
}

void ast_print(struct astnode *_node, size_t level)
{
        struct astnode *node = UNWRAP(_node);

        char indent[level + 1];
        memset(indent, '\t', level);
        indent[level] = '\0';


#define INDENTED(...) \
        printf("?\t%s", indent); \
        printf(__VA_ARGS__)

        if (!node) {
                INDENTED("( Null )\n");
                return;
        }

        size_t line = node->line;

#undef INDENTED
#define INDENTED(...) \
        printf("%ld\t%s", line, indent); \
        printf(__VA_ARGS__)

        char *s;

        switch (node->type) {
                case NODE_UNDEFINED:
                INDENTED("( Undefined )\n");
                        break;

                case NODE_NOTHING:
                INDENTED("Nothing\n");
                        break;
                case NODE_BLOCK:
                INDENTED("Block:\n");
                        ast_print(node->block.nodes, level + 1);
//                        ast_print(node->block.symbols, level + 1);
                        break;
                case NODE_COMPOUND:
                INDENTED("Compound:\n");
                        for (size_t i = 0; i < node->node_compound.count; i++)
                                ast_print(node->node_compound.array[i], level + 1);
                        break;
                case NODE_PROGRAM:
                INDENTED("Program:\n");
                        ast_print(node->program.block, level + 1);
                        break;
                case NODE_INTEGER_LITERAL:
                INDENTED("Integer Literal: %lld\n", node->integer_literal.integerValue);
                        break;
                case NODE_FLOAT_LITERAL:
                INDENTED("Float Literal: %f\n", node->float_literal.floatValue);
                        break;

                case NODE_STRING_LITERAL:
                INDENTED("String literal (%ld) '%s'\n", node->string_literal.length, node->string_literal.value);
                        break;

                case NODE_BINARY_OP:
                INDENTED("Binary operation %s:\n", binaryop_string(node->binary.op));
                        ast_print(node->binary.left, level + 1);
                        ast_print(node->binary.right, level + 1);
                        break;

                case NODE_VARIABLE_DECL:
                        if (node->declaration.constant) {
                                INDENTED("Stable Variable Declaration");
                        } else {
                                INDENTED("Variable Declaration");
                        }

                        s = astdtype_string(node->declaration.type);

                        printf(" '%s' [%s]", node->declaration.identifier, s);

                        free(s);

                        if (!node->declaration.value) {
                                printf("\n");
                                break;
                        }

                        printf(":\n");
                        ast_print(node->declaration.value, level + 1);
                        break;

                case NODE_POINTER:
                INDENTED("Pointer to:\n");
                        ast_print(node->pointer.target, level + 1);
                        break;

                case NODE_VARIABLE_USE:
                INDENTED("Variable Use '%s'\n", node->variable.identifier);
                        break;

                case NODE_VARIABLE_ASSIGNMENT:
                INDENTED("Variable Assignment '%s':\n", node->assignment.identifier);
                        ast_print(node->assignment.value, level + 1);
                        break;

                case NODE_FUNCTION_DEFINITION:
                        s = astdtype_string(node->function_def.type);
                        INDENTED("Function Definition '%s' (%s) of %s:\n", FUNCTION_ID(node->function_def.identifier),
                                 (node->function_def.generated
                                  ? node->function_def.generated->generated_function.generated_id : "Not yet analyzed"),
                                 s);
                        free(s);
                        ast_print(node->function_def.params, level + 1);
                        if (node->function_def.capture)
                                ast_print(node->function_def.capture, level + 1);
                        ast_print(node->function_def.block, level + 1);
                        ast_print(node->function_def.attributes, level + 1);
                        break;

                case NODE_FUNCTION_CALL:
                INDENTED("Function Call '%s':\n", node->function_call.identifier);
                        ast_print(node->function_call.values, level + 1);
                        break;

                case NODE_ATTRIBUTE:
                INDENTED("Attribute (%s)\n", node->attribute.identifier);
                        break;

                case NODE_RESOLVE:
                INDENTED("Resolve (%s):\n",
                         node->resolve.function ? FUNCTION_ID(node->resolve.function->function_def.identifier)
                                                : "Not yet analyzed");
                        ast_print(node->resolve.value, level + 1);
                        break;

                case NODE_IF:
                INDENTED("Conditional If:\n");
                        ast_print(node->if_statement.expr, level + 1);
                        ast_print(node->if_statement.block, level + 1);
                        ast_print(node->if_statement.next_branch, level + 1);
                        break;

                case NODE_INCLUDE:
                INDENTED("Include \"%s\"\n", node->include.path);
                        break;

                case NODE_SYMBOL:
                INDENTED("Symbol (%s)\n", node->symbol.identifier);
                        break;

                case NODE_VOID_PLACEHOLDER:
                INDENTED("( Void - Nothing )\n");
                        break;

                case NODE_PRESENT_FUNCTION:
                        s = astdtype_string(node->present_function.type);
                        INDENTED("Linked function \"%s\" (%s):\n", node->present_function.identifier, s);
                        free(s);
                        ast_print(node->present_function.params, level + 1);
                        break;

                default:
                INDENTED("( Incorrect node type )\n");
                        break;
        }

#undef INDENTED
}
