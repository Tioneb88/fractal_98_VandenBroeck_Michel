#include <stdlib.h>
#include "fractal.h"

struct fractal *fractal_new(const char *name, int width, int height, double a, double b)
{
	fractal *f = malloc(sizeof(fractal));
	if (fractal == NULL) 
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
	return fractal_compute_value(f,x,y);
}

void fractal_set_value(struct fractal *f, int x, int y, int val)
{
    /* TODO */
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
