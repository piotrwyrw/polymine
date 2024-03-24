//
// Created by Piotr Krzysztof Wyrwas on 24.03.24.
//

#include "util.h"

#include <string.h>
#include <math.h>

double char_to_digit(char c)
{
        return (double) (c - '0');
}

double string_to_number(char *str)
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
                        output = output * 10 + char_to_digit(c);
                else
                        output = output + (char_to_digit(c) / pow(10, (double) dIdx++));
        }

        return output;
}

void ast_print(struct astnode *node, size_t level)
{
        char indent[level + 1];
        memset(indent, '\t', level);
        indent[level] = '\0';


#define INDENTED(...) \
        printf("%s", indent); \
        printf(__VA_ARGS__)

        if (!node) {
                INDENTED("( Null )\n");
                return;
        }

        size_t line = node->line;

#undef INDENTED
#define INDENTED(...) \
        printf("%ld\t", line);              \
        printf("%s", indent); \
        printf(__VA_ARGS__)

        switch (node->type) {
                case NODE_UNDEFINED:
                INDENTED("( Undefined )\n");
                        break;

                case NODE_NOTHING:
                INDENTED("Nothing\n");
                        break;
                case NODE_BLOCK:
                INDENTED("Block:\n");
                        for (size_t i = 0; i < node->block.count; i++)
                                ast_print(node->block.nodes[i], level + 1);
                        break;
                case NODE_PROGRAM:
                INDENTED("Program:\n");
                        ast_print(node->program.block, level + 1);
                        break;
                case NODE_NUMBER_LITERAL:
                INDENTED("Number Literal: %f\n", node->number_literal.value);
                        break;

                case NODE_STRING_LITERAL:
                INDENTED("String literal (%ld): %s\n", node->string_literal.length, node->string_literal.value);
                        break;

                case NODE_BINARY_OP:
                INDENTED("Binary operation (%s):\n", binaryop_string(node->binary.op));
                        ast_print(node->binary.left, level + 1);
                        ast_print(node->binary.right, level + 1);
                        break;

                default:
                INDENTED("( Incorrect node type )\n");
                        break;
        }

#undef INDENTED
}
