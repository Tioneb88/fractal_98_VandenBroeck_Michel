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

int read(char* filename)
{
	FILE* fichier = NULL;

	fichier = fopen(filename, "r");

	if (fichier != NULL)
	{
		while (fgets(line, MAX_SIZE, fichier) != NULL) // On lit le fichier tant qu'on ne reçoit pas d'erreur (NULL).
		{
			fractal fract = extract(line);

			// On met la fractale extraite dans le buffer où elle va attendre son calcul.
			if (fract != NULL)
			{

			}
		}

		fclose(fichier);
	}

	return 0;
}

fractal extract(char* line)
{
	int iter = 0;
	char *token;
	fractal fract = new fractal(NULL, 0, 0, 0.0, 0.0);

	// On obtient le premier mot de la ligne.
	token = strtok(line, " ");

	// On ignore les lignes invalides.
	if (token[0] == '#' || token[0] == " " || token == NULL)
	{
		return NULL;
	}

	// On récupère tous les mots de la ligne qu'on stocke dans la structure fractal.
	while(token != NULL) {
		switch (iter) {
		case 0:
			char *copy = strlencopy(token);
			fract->name = copy;
			break;
		case 1:
			fract->width = atoi(token);
			break;
		case 2:
			fract->height = atoi(token);
			break;
		case 3:
			fract->a = atof(token);
			break;
		case 4:
			fract->b = atof(token);
			break;
		default :
			return NULL;
		}

		token = strtok(NULL, " ");
		iter++;
	}
	if (iter < 5)
	{
		return NULL;
	}
	return fract;
}
