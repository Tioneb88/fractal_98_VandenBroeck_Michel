#include <stdlib.h>
#include <string.h>
#include "fractal.h"

#define MAX_SIZE 1000

struct fractal *fractal_new(const char *name, int width, int height, double a, double b)
{
	fractal *f = malloc(sizeof(fractal));
	if (f == NULL)
		return NULL;
	f->name = name;
	f->width = width;
	f->height = height;
	f->a = a;
	f->b = b;
    return f;
}

void fractal_free(struct fractal *f)
{
	free(f->name);
	free(f->pixel);
	free(f);
}

const char *fractal_get_name(const struct fractal *f)
{
	if (f == NULL)
		return NULL;
	return f->name;
}

int fractal_get_value(const struct fractal *f, int x, int y)
{
	return f->pixel[x][y];
}

void fractal_set_value(struct fractal *f, int x, int y, int val)
{
	f->pixel[x][y] = val;
}

int fractal_get_width(const struct fractal *f)
{
	if (f == NULL)
		return NULL;
	return f->width;
}

int fractal_get_height(const struct fractal *f)
{
	if (f == NULL)
		return NULL;
	return f->height;
}

double fractal_get_a(const struct fractal *f)
{
	if (f == NULL)
		return NULL;
	return f->a;
}

double fractal_get_b(const struct fractal *f)
{
	if (f == NULL)
		return NULL;
	return f->b;
}

double fractal_get_average(const struc fractal *f) {
	if (f == NULL)
		return NULL;
	return f->average;
}
