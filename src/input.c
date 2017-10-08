#include "lemon.h"
#include "arena.h"
#include "input.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>

#ifdef LINUX
#include <linux/limits.h>
#endif

struct input *
input_create(struct lemon *lemon)
{
	struct input *input;

	input = arena_alloc(lemon, lemon->l_arena, sizeof(*input));
	if (!input) {
		return NULL;
	}
	memset(input, 0, sizeof(*input));

	return input;
}

void
input_destroy(struct lemon *lemon, struct input *input)
{
}

int
input_set_file(struct lemon *lemon, const char *filename)
{
	FILE *fp;
	long size;
	struct input *input;

	fp = fopen(filename, "r");
	if (!fp) {
		return 0;
	}

	fseek(fp, 0, SEEK_END);

	input = lemon->l_input;
	input->size = ftell(fp) + 1;
	input->buffer = arena_alloc(lemon, lemon->l_arena, input->size);
	memset(input->buffer, 0, input->size);

	fseek(fp, 0, SEEK_SET);
	size = fread(input->buffer, 1, (size_t)input->size, fp);
	if (size != input->size - 1) {
		fclose(fp);

		return 0;
	}
	fclose(fp);

	input->filename = arena_alloc(lemon, lemon->l_arena, PATH_MAX);
	strncpy(input->filename, filename, PATH_MAX);
	input->offset = 0;

	return 1;
}

int
lemon_input_set_file(struct lemon *lemon, const char *filename)
{
        return input_set_file(lemon, filename);
}

int
input_set_buffer(struct lemon *lemon,
                 const char *filename,
                 char *buffer,
                 int length)
{
	struct input *input;

	input = lemon->l_input;
	input->size = length;
	input->buffer = buffer;
	input->filename = arena_alloc(lemon, lemon->l_arena, PATH_MAX);
	strncpy(input->filename, filename, PATH_MAX);
	input->offset = 0;

	return 1;
}

int
lemon_input_set_buffer(struct lemon *lemon,
                       const char *filename,
                       char *buffer,
                       int length)
{
        return input_set_buffer(lemon, filename, buffer, length);
}

char *
input_filename(struct lemon *lemon)
{
	struct input *input;

	input = lemon->l_input;
	return input->filename;
}

long
input_line(struct lemon *lemon)
{
	struct input *input;

	input = lemon->l_input;
	return input->line;
}

long
input_column(struct lemon *lemon)
{
	struct input *input;

	input = lemon->l_input;
	return input->column;
}

int
input_getchar(struct lemon *lemon)
{
	int c;
	struct input *input;

	input = lemon->l_input;
	if (input->offset >= input->size) {
		return 0;
	}

	c = input->buffer[input->offset++];
	if (c == '\n') {
		input->line++;
		input->column = 0;
	} else {
		input->column++;
	}

	return c;
}

void
input_ungetchar(struct lemon *lemon, int c)
{
	struct input *input;

	input = lemon->l_input;
	input->offset--;
}
