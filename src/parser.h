#ifndef LEMON_PARSER_H
#define LEMON_PARSER_H

struct lemon;
struct syntax;

struct syntax *
parser_parse(struct lemon *lemon);

#endif /* LEMON_PARSER_H */
