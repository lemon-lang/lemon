#ifndef LEMON_LEXER_H
#define LEMON_LEXER_H

struct lemon;

struct lexer {
	int lookahead;

	long length;
	char *buffer; /* name, number and string's text value */
};

struct lexer *
lexer_create(struct lemon *lemon);

void
lexer_destroy(struct lemon *lemon, struct lexer *lexer);

long
lexer_get_length(struct lemon *lemon);

char *
lexer_get_buffer(struct lemon *lemon);

int
lexer_get_token(struct lemon *lemon);

int
lexer_next_token(struct lemon *lemon);

#endif /* LEMON_LEXER_H */
