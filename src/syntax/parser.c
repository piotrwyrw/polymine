#include "parser.h"
#include "lexer.h"

#include <string.h>

void parser_init(struct parser *p, struct lexer *lx)
{
        p->lx = lx;
        p->line = 1;
        p->block = NULL;

        p->types = astnode_empty_compound(0, NULL);

        lxtok_init(&p->current, LX_UNDEFINED, NULL, 1);
        lxtok_init(&p->next, LX_UNDEFINED, NULL, 1);

        parser_advance(p);
        parser_advance(p);
}

void parser_free(struct parser *p)
{
        lxtok_free(&p->current);
        lxtok_free(&p->next);
        freeDataTypes = true;
        astnode_free_compound(p->types);
}

void parser_advance(struct parser *p)
{
        lxtok_free(&p->current);
        p->current = p->next;

        if (p->current.line > p->line)
                p->line = p->current.line;

        if (lexer_empty(p->lx)) {
                lxtok_init(&p->next, LX_UNDEFINED, NULL, 0);
                return;
        }

        lexer_next(p->lx, &p->next);
}