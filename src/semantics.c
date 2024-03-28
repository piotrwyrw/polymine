#include "semantics.h"

#include <stdbool.h>
#include <string.h>

static struct astnode *filter_variable(char *id, struct astnode *node)
{
        if (node->type != NODE_VARIABLE_SYMBOL)
                return NULL;

        if (strcmp(node->variable_symbol.identifier, id) != 0)
                return NULL;

        return node;
}

struct astnode *find_symbol(char *id, struct astnode *block)
{
        if (block->type != NODE_BLOCK) {
                printf("find_symbol(%s): 'block' is a %s!\n", id, nodetype_string(block->type));
                return NULL;
        }

        // Traverse the tree upwards to find the first block that contains said symbol
        struct astnode *b = block;
        struct astnode *symbol;

        while (b != NULL) {
                if ((symbol = astnode_compound_foreach(block->block.symbols, id, (void *) filter_variable)))
                        return symbol;

                b = b->super;
        }

        return NULL;
}

void put_symbol(struct astnode *block, struct astnode *symbol)
{
        astnode_push_compound(block->block.symbols, symbol);
}

inline _Bool analyze_program(struct astnode *program)
{
        analyze_any(program->program.block);
}

_Bool analyze_compound(struct astnode *compound)
{
        for (size_t i = 0; i < compound->node_compound.count; i++)
                if (!analyze_any(compound->node_compound.array[i]))
                        return false;

        return true;
}

_Bool analyze_any(struct astnode *node)
{
        switch (node->type) {
                case NODE_BLOCK:
                        return analyze_compound(node->block.nodes);
                case NODE_VARIABLE_DECL:
                        return analyze_variable_declaration(node);
                default:
                        return true;
        }
}

_Bool analyze_variable_declaration(struct astnode *decl)
{
        struct astnode *symbol;

        if ((symbol = find_symbol(decl->declaration.identifier, decl->super))) {
                printf("Redefinition of variable '%s' on line %ld. Original definition on line %ld.\n", decl->declaration.identifier, decl->line, symbol->variable_symbol.declaration->line);
                return false;
        }

        put_symbol(decl->super, astnode_variable_symbol(decl->super, decl->declaration.identifier, decl->declaration.type, decl));

        return true;
}

_Bool analyze_binary_expression(struct astnode *bin)
{
        return true;
}

_Bool analyze_variable_use(struct astnode *use)
{
        return true;
}