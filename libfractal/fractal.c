#include <stdlib.h>
#include <string.h>
#include "fractal.h"

#define MAX_SIZE 1000

struct fractal *fractal_new(const char *name, int width, int height, double a, double b)
{
	fractal *f =(fractal *) malloc(sizeof(fractal));
	if (f == NULL)
		return NULL;
	f->name = name;
	f->width = width;
	f->height = height;
	f->a = a;
	f->b = b;
	f->pixel = malloc(width*height*sizeof(int));
	if(f->pixel == NULL)
	  {
	    return NULL;
	  }
		f->average = 0;
    return f;
}

void fractal_free(struct fractal *f)
{
	free(f->pixel;)
	free(f);
}

const char *fractal_get_name(const struct fractal *f)
{
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
	return f->width;
}

int fractal_get_height(const struct fractal *f)
{
	return f->height;
}

double fractal_get_a(const struct fractal *f)
{
	return f->a;
}

double fractal_get_b(const struct fractal *f)
{
	return f->b;
}

double fractal_get_average(const struct fractal *f) {
	return f->average;
}
