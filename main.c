#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include "fractal.h"

#define READ_SIZE 200
#define COMPUTE_SIZE 30

// fonctions externes
int read(char* filename);
fractal extract(char* line);
void thread_compute();
void thread_compare();
void fractal_average(const struct fractal *f);

// variables globales
fractal *readbuffer[READ_SIZE];
fractal *computingbuffer[COMPUTE_SIZE];
fractal *max_frac;

// variables globales pour le readbuffer (problème des producteurs-consommateurs)
pthread_mutex_t readbufferMutex;
sem_t readbufferEmpty;
sem_t readbufferFull;

// variables globales pour le computebuffer (problème des producteurs-consommateurs)
pthread_mutex_t computebufferMutex;
sem_t computebufferEmpty;
sem_t computebufferFull;

int main(int argc, char *argv[])
{
	/*
	 * 1ère étape : récupération des arguments et des options de la ligne de commande.
	 */

	// On récupère les options et les arguments passés en ligne de commande.
	int dflag = 0;
	int maxflag = 0;
	int maxthread = 1;
	int error;

	while ((c = getopt(argc, argv, "maxthreads:d")) != -1)
		switch (c)
		{
		case 'd':
			dflag = 1;
			break;
		case 'maxthreads':
		    maxflag = 1;
			maxthread = optarg;
			break;
		case '?':
			if (optopt == 'maxthreads')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr,
					"Unknown option character `\\x%x'.\n",
					optopt);
			return 1;
		default:
			abort();
		}

	/*
	* 2ème étape : on lance un thread de lecture par fichier ou pour l'entrée standard (avec un maximum de 200 threads).
	* On extrait les données de fractales des lignes valides, on stocke ces données dans une structure fractale dont l'
	* adresse est placée dans un buffer de lecture dès qu'il y a suffisamment de place.
	*/

	//initialisation des protecteurs d'accès à readbuffer
	int readMerr = pthread_mutex_init(&readbufferMutex, NULL);
	if(readMerr != 0)
    {
        error(readMerr, "pthread_mutex_init_read");
    }
	sem_init(&readbufferEmpty, 0, READ_SIZE);
	sem_init(&readbufferFull, 0, 0);

    //lecture des fichiers et remplissage du readbuffer (=producteurs)
    pthread_t readthreads[READ_SIZE];
	int readerr;
	for (long i = 0; i < READ_SIZE; i++)
	{
		readerr = pthread_create(&readthreads[i], NULL, &read, (void *) &fileNames[i]);
		if (readerr != 0)
		{
			error(readerr, "pthread_create_read");
		}
	}

	/*
	for(long j = 0; j < READ_SIZE; j++)
    {
        readerr = pthread_join(&readthreads[i], NULL);
		if (readerr != 0)
		{
			error(readerr, "pthread_join_read");
		}
    }
    */

	/*
	* 3ème étape : on va voir si le readbuffer contient au moins un élément, si c'est le cas et qu'on
	* n'a pas encore atteint le maximum de threads de calcul alors, on lance un thread qui récupère
	* la fractale stockée, calcule la valeur de chacun de ses pixels ainsi que la moyenne des valeurs
	* de tous ses pixels et regarde si cette moyenne est plus grande que le maximum (si c'est le cas on
	* remplace le maximum) puis, place la fractale dans un buffer de calcul dès qu'il y a suffisamment de
	* place. Le maximum n'est nécessaire que dans le cas où l'argument -d n'est pas présent.
	*/

	//lecture du readbuffer (=consommateurs) puis remplissage du computebuffer (=producteurs)
	int computeSize = 1;
	if(maxthread<COMPUTE_SIZE)
    {
        computeSize = maxthread;
    }
    else
    {
        computeSize = COMPUTE_SIZE;
    }
	pthread_t computethreads[computeSize];

	//initialisation des protecteurs d'accès à computebuffer
	int computeMerr = pthread_mutex_init(&computebufferMutex, NULL);
	if(computeMerr != 0)
    {
        error(computeMerr, "pthread_mutex_init_compute");
    }
	sem_init(&computebufferEmpty, 0, computeSize);
	sem_init(&computebufferFull, 0, 0);

	//lecture de readbuffer (=consommateurs) et remplissage du computebuffer (=producteurs)
	int computeerr;
	for (long k = 0; k < computeSize; k++) {
		computeerr = pthread_create(&(computethreads[k], NULL, &thread_compute, (void *) &computeSize));
			if (computeerr != 0) {
				error(computeerr, "pthread_create_compute")
			}
	}

	/*
	for(long m = 0; m < NFiles; m++)
    {
        computeerr = pthread_join(&readthreads[m], NULL);
		if (computeerr != 0)
		{
			error(computeerr, "pthread_join_compute");
		}
    }
    */

    /*
	* 4ème étape : on affiche la fractale maximale ou toutes les fractales si l'argument -d est présent.
	*/

    if (dflag != 1)
    {
        pthread_t comparethread;
        int compareerr = pthread_create(&(comparethread, NULL, &thread_compare, (void *) &computeSize));
        if (compareerr != 0) {
            error(compareerr, "pthread_create_compare");
        }
        write_bitmap_sdl(max_frac, fractal_get_name(max_frac));
    }
    else{
        pthread_t displaythread;
        int displayerr = pthread_create(&(displaythread, NULL, &thread_compute_get, (void *) &computeSize));
        if (displayerr != 0) {
            error(displayerr, "pthread_create_display");
        }

        fractal *frac;
        int join_displayerr = pthread_join(&displaythread, (void **) &frac);
        if (join_displayeerr != 0)
        {
            error(join_displayerr, "pthread_join_display");
        }
        write_bitmap_sdl(frac, fractal_get_name(frac));
    }

    //élimination des variables de protection des buffers
    int err_destroy = sem_destroy(&readbufferEmpty);
    if (err_destroy != 0){
            error(err_destroy, "sem_destroy_err");
        }
    err_destroy = sem_destroy(&readbufferFull);
    if (err_destroy != 0){
            error(err_destroy, "sem_destroy_err");
        }
    err_destroy = sem_destroy(&computebufferEmpty);
    if (err_destroy != 0){
            error(err_destroy, "sem_destroy_err");
        }
    err_destroy = sem_destroy(&computebufferFull);
    if (err_destroy != 0){
            error(err_destroy, "sem_destroy_err");
        }

    int err_destroy_mutex = pthread_mutex_destroy(&readbufferMutex);
    if (err_destroy_muetx != 0){
            error(err_destroy_mutex, "mutex_destroy_err");
        }
    int err_destroy_mutex = pthread_mutex_destroy(&computebufferMutex);
    if (err_destroy_muetx != 0){
            error(err_destroy_mutex, "mutex_destroy_err");
        }



	return 0;
}








/*
* read
*
* Lit le fichier passé en argument, extrait chaque ligne et appelle la fonction extract
* avec la ligne courante en argument. Stocke ensuite la fractale retournée dans le buffer
* s'il y a encore assez de place, attend que la place se libère sinon.
*
* @filename: nom du fichier dont on extrait les lignes.
* @return: 0 si on finit la lecture du fichier sans accroc, -1 autrement.
*/
void *thread_read(char* filename)
{
	FILE* fichier = NULL;

	fichier = fopen(filename, "r");

	if (fichier != NULL)
	{
		while (fgets(line, MAX_SIZE, fichier) != NULL) // On lit le fichier tant qu'on ne reçoit pas d'erreur (NULL).
		{
			fractal fract = line_extract(line);

			// On met la fractale extraite dans le buffer où elle va attendre son calcul.
			if (fract != NULL)
			{
			    int i;
			    int found = 0;
			    sem_wait(&readbufferEmpty); //attente d'un slot libre dans readbuffer.
			    pthread_mutex_lock(&readbufferMutex);
			    //section critique
			    for(i=0; i<READ_SIZE && found == 0; i++)
                {
                    if(readbuffer[i] == NULL)
                    {
                        readbuffer[i] = fract;
                        found = 1;
                    }
                }
			    //fin de section critique
			    pthread_mutex_unlock(&readbufferMutex);
			    sem_post(&readbufferFull);
			}
		}
		fclose(fichier);
	}
}

/*
 * extract
 *
 * Lit la chaîne de caractères extraite d'un fichier et extrait les informations pour les
 * stocker dans une structure fractale. null est renvoyé si le format "nom largeur hauteur
 * partie_réelle partie_imaginaire" n'est pas respecté.
 *
 * @line: chaîne de caractères dont on doit extraire l'information
 * @return: une structure fractal contenant les informations ou null si une erreur survient.
 */
fractal line_extract(char* line)
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

/*
 * thread_compute
 *
 * Lit le buffer de lecture, s'il n'est pas vide, prend une fractale qui y est stockée, calcule
 * la valeur de chacun de ses pixels et la moyenne des valeurs de tous les pixels puis, place la
 * structure fractale ainsi modifiée dans le buffer de calcul dès que celui-ci dispose d'une place
 * libre.
 *
 * @return: void
 */
void *thread_compute(int computeSize) {
    struct fractal *fcopy;
    int found = 0;
    int i;
	sem_wait(&readbufferFull); //attente d'un slot rempli dans readbuffer
	pthread_mutex_lock(&readbufferMutex);
    //section critique
	for(i=0; i<READ_SIZE && found == 0; i++)
    {
        if(readbuffer[i] != NULL)
        {
            fcopy = readbuffer[i];
            readbuffer[i] = NULL;
            found = 1;
        }
    }
	//fin de section critique
    pthread_mutex_unlock(&readbufferMutex);
    sem_post(&readbufferEmpty);

	int w = fractal_get_width(f);
	int h = fractal_get_height(f);
	int x;
	int y;

	for (x = 0; x < w; x++) {
		for (y = 0; y < h; y++) {
			int val = fractal_compute_value(f, x, y);
			fractal_set_value(fcopy, x, y, val);
		}
	}

	fcopy->average = fractal_average(fcopy);

    int j;
    int foundBis = 0;
	sem_wait(&computebufferEmpty); //attente d'un slot libre dans computebuffer.
    pthread_mutex_lock(&computebufferMutex);
    //section critique
    for(j=0; j<computeSize && foundBis == 0; j++)
    {
        if(computebuffer[j] == NULL)
        {
            computebuffer[j] = fcopy;
            foundBis = 1;
        }
    }
    //fin de section critique
    pthread_mutex_unlock(&computebufferMutex);
    sem_post(&computebufferFull);
}

/*
 * fractal_average
 *
 * Calcule la moyenne de la valeur de tous les pixels de la structure fractale et la
 * stocke dans la fractale.
 *
 * @f: structure fractale dont on calcule la moyenne des pixels.
 * @return: void
 */
void fractal_average(const struct fractal *f) {
	int sum = 0;
	int w = fractal_get_width(f);
	int h = fractal_get_height(f);
	int x;
	int y;

	for (x = 0; x < w; x++) {
		for (y = 0; y < h; y++) {
			sum += fractal_get_value(f,x,y);
		}
	}
	f->average = (sum / (w*h));
}

/*
 * thread_compare
 *
 * Lit le buffer de calcul, s'il n'est pas vide, prend une fractale qui y est stockée, regarde si la
 * moyenne des valeurs de tous ses pixels est plus grande que la moyenne maximale actuelle, si c'est
 * le cas, remplace la fractale maximale par la fractale courante.
 *
 * @return: void
 */
void thread_compare(int computeSize) {
    struct fractal *fcopy;
    int i;
    int found = 0;
	sem_wait(&computebufferFull); //attente d'un slot rempli dans computebuffer
	pthread_mutex_lock(&computebufferMutex);
    //section critique
	for(i=0; i<computeSize && found == 0; i++)
    {
        if(computebuffer[i] != NULL)
        {
            fcopy = computebuffer[i];
            computebuffer[i] = NULL;
            found = 1;
        }
    }
	//fin de section critique
    pthread_mutex_unlock(&computebufferMutex);
    sem_post(&computebufferEmpty);

	if (fcopy->average > max_frac->average)
    {
		max_frac = fcopy;
	}
}

/*
 * thread_compute_get
 *
 * Lit le buffer de calcul, s'il n'est pas vide, prend une fractale qui y est stockée et la retourne.
 *
 * @return: pointeur casté de la structure fractale lue.
 */
void *thread_compute_get(int computeSize)
{
    struct fractal *fcopy;
    int i;
    int found = 0;
	sem_wait(&computebufferFull); //attente d'un slot rempli dans computebuffer
	pthread_mutex_lock(&computebufferMutex);
    //section critique
	for(i=0; i<computeSize && found == 0; i++)
    {
        if(computebuffer[i] != NULL)
        {
            fcopy = computebuffer[i];
            computebuffer[i] = NULL;
            found = 1;
        }
    }
	//fin de section critique
    pthread_mutex_unlock(&computebufferMutex);
    sem_post(&computebufferEmpty);

    return ((void *) fcopy);
}
