#include "parser.h"
#include "ast.h"
#include "lexer.h"

#include <string.h>

void parser_init(struct parser *p, struct lexer *lx)
{
        p->lx = lx;
        p->line = 1;

        lxtok_init(&p->current, LX_UNDEFINED, NULL, 1);
        lxtok_init(&p->next, LX_UNDEFINED, NULL, 1);

        parser_advance(p);
        parser_advance(p);
}

void parser_Free(struct parser *p)
{
        lxtok_free(&p->current);
        lxtok_free(&p->next);
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

struct astdtype *parse_type(struct parser *p)
{
        char *identifier;

        if (p->current.type != LX_IDEN) {
                printf("Expected type name or functional identifier. Got %s (\"%s\") on line %ld.\n",
                       lxtype_string(p->current.type), p->current.value, p->line);
                return NULL;
        }

        identifier = p->current.value;

        enum builtin_type bt = builtin_from_string(identifier);

        if (bt != BUILTIN_UNDEFINED) {
                parser_advance(p);
                return astdtype_builtin(bt);
        }

        if (strcmp(identifier, "ptr") == 0) {
                parser_advance(p);

                if (p->current.type != LX_LPAREN) {
                        printf("Expected '(' after functional identifier. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        return NULL;
                }

                parser_advance(p);

                struct astdtype *enclosed = parse_type(p);

                if (!enclosed)
                        return NULL;

                if (p->current.type != LX_RPAREN) {
                        printf("Expected ')' after enclosed type. Got %s (\"%s\") on line %ld.\n",
                               lxtype_string(p->current.type), p->current.value, p->line);
                        astdtype_free(enclosed);
                        return NULL;
                }

                parser_advance(p);

                return astdtype_pointer(enclosed);
        }

        struct astdtype *type = astdtype_custom(identifier);
        parser_advance(p);
        return type;
}

struct astnode *parse_string_literal(struct parser *p)
{
        if (p->current.type != LX_STRING) {
                printf("Expected string literal, but found %s (\"%s\") on line %ld.\n", lxtype_string(p->current.type),
                       p->current.value, p->line);
                return NULL;
        }

        return astnode_string_literal(p->line, p->current.value);;
}
