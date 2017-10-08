#ifndef LEMON_INPUT_H
#define LEMON_INPUT_H

struct lemon;

struct input {
	long size;   /* size of buffer */
	long offset; /* offset in buffer */
	long line;   /* line number */
	long column; /* position of current line */
	char *filename;

	char *buffer;
};

struct input *
input_create(struct lemon *lemon);

void
input_destroy(struct lemon *lemon,
              struct input *input);

int
input_set_file(struct lemon *lemon,
               const char *filename);

int
input_set_buffer(struct lemon *lemon,
                 const char *filename,
                 char *buffer,
                 int length);

char *
input_filename(struct lemon *lemon);

long
input_line(struct lemon *lemon);

long
input_column(struct lemon *lemon);

int
input_getchar(struct lemon *lemon);

void
input_ungetchar(struct lemon *lemon, int c);

#endif /* LEMON_INPUT_H */
