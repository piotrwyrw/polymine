#include "parser.h"
#include "ast.h"
#include "lexer.h"

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

struct astnode *parse_string_literal(struct parser *p)
{
	if (p->current.type != LX_STRING) {
		printf("Expected string literal, but found %s on line %ld.\n", lxtype_string(p->current.type), p->line);
		return NULL;
	}

	return astnode_string_literal(p->line, p->current.value);;
}
