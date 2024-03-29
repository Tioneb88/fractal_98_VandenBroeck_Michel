#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <string.h>
#include <error.h>
#include "libfractal/fractal.h"

// definition de la taille maximale raisonnable des buffers de lecture et de calcul pour eviter de depasser les capacites de la machine.
#define READ_SIZE 100
#define COMPUTE_SIZE 10

// fonctions externes définies sous la main et utilisees dans celle-ci.
void *thread_read(char* filename);
fractal *line_extract(char* line);
void *thread_compute(int computeSize);
double fractal_average(const struct fractal *f);
void *thread_compare(int computeSize);
void *thread_compute_get(int computeSize);
int find_name(struct fractal *f, char *namelist[64]);

// variables globales.
fractal **readbuffer; // buffer de lecture (les pointeurs des fractales sont stockes apres avoir ete lues)
fractal **computingbuffer; // buffer de calcul (les pointeurs des fractales sont stockes apres que tous leurs pixels aient ete calcules )
fractal *max_frac; // fractale dont la moyenne des valeurs de tous les pixels est la plus grande (utile si l option -d n est pas active)
char **name_list; // liste des noms de fractales deja utilises
int count = 0;

// variables globales pour la protection du readbuffer (probleme des producteurs-consommateurs).
pthread_mutex_t readbufferMutex;
sem_t readbufferEmpty;
sem_t readbufferFull;

// variables globales pour la protection du computingbuffer (probleme des producteurs-consommateurs).
pthread_mutex_t computebufferMutex;
sem_t computebufferEmpty;
sem_t computebufferFull;

int main(int argc, char *argv[])
{
	// malloc de la liste des noms de fractales deja utilises
	name_list = (char **) malloc(sizeof(char *)*1000000);
	if(name_list == NULL)
	{
		 error(-1,0, "name_list_malloc");
	}

	// malloc de la fractale maximale
	max_frac = (fractal *) malloc(sizeof(fractal));
	if(max_frac == NULL)
	{
		 error(-1,0, "max_frac_malloc");
	}


	/*
	 * 1ere etape : recuperation des options de la ligne de commande.
	 */


	int fichiers = argc - 1;	// nombre de fichiers a lire (ligne de commande comprise). On retire deja 1 car argv[argc-1] contient fichierOut.
	int dflag = 0;	// drapeau de presence de l option -d
	int maxflag = 0; // drapeau de presence de l option --maxthreads
	int maxthread = 1; // nombre maximal de threads de calculs autorises (minimum 1)

	// Recuperation des options et mise a jour des drapeaux
	int a;
	for(a=1;a<fichiers;a++){
		if(strcmp(argv[a],"-d")== 0){
			dflag = 1;
		}
		if(strcmp(argv[a],"--maxthreads")== 0){
			maxflag = 1;
			maxthread = atoi(argv[a+1]);
		}
	}
	// indice du premier fichier a lire
	int decalage = dflag + 2*maxflag + 1; // +1 car on sait que arg[0] contient le nom de l'executable.


	/*
	* 2eme etape : on lance un thread de lecture par fichier ou pour l'entree standard (avec un maximum de @READ_SIZE threads).
	* On extrait les donnees de fractales des lignes valides, on stocke ces donnees dans une structure fractale dont l adresse
	* est placee dans un buffer de lecture (@readbuffer) des qu il y a suffisamment de place.
	*/


	// initialisation des protecteurs d'acces a readbuffer
	int readMutexErr = pthread_mutex_init(&readbufferMutex, NULL);
	if(readMutexErr != 0)
    {
			error(readMutexErr,0, "mutex_init_read");
    }
	int readSemEmptyErr = sem_init(&readbufferEmpty, 0, READ_SIZE);
	if(readSemEmptyErr != 0)
    {
			error(readSemEmptyErr,0, "sem_empty_init_read");
    }
	int readSemFullErr = sem_init(&readbufferFull, 0, 0);
	if(readSemFullErr != 0)
    {
			error(readSemFullErr,0, "sem_full_init_read");
    }

  // definition de la taille de readbuffer
	int readSize = 1;
	if(fichiers-decalage<READ_SIZE)
    {
        readSize = fichiers-decalage;
    }
    else
    {
        readSize = COMPUTE_SIZE;
    }

	// malloc du readbuffer
	*readbuffer = (fractal *) malloc(readSize*sizeof(fractal *));
	if(readbuffer == NULL)
	{
		error(-1,0, "readbuffer_malloc");
	}

	// lecture des fichiers et remplissage du readbuffer (= threads producteurs)
  pthread_t readThreads[readSize];
	int readErr;
	long i;
	for (i = decalage; i < argc-1 && i < readSize; i++)
	{
		readErr = pthread_create(&readThreads[i], NULL,(void *) &thread_read, (void *) argv[i]);
		if (readErr != 0)
		{
			error(readErr,0, "pthread_create_read");
		}
	}


	/*
	* 3eme etape : on va voir si le readbuffer contient au moins un element, si c est le cas et qu on
	* n a pas encore atteint le maximum de threads de calcul alors, on lance un thread qui recupere
	* la fractale stockee, calcule la valeur de chacun de ses pixels ainsi que la moyenne des valeurs
	* de tous ses pixels puis, place la fractale dans un buffer de calcul des qu il y a suffisamment de
	* place.
	*/


	// definition de la taille de computingbuffer
	int computeSize = 1;
	if(maxthread<COMPUTE_SIZE && maxflag == 1)
    {
        computeSize = maxthread;
    }
    else
    {
        computeSize = COMPUTE_SIZE;
    }

		// malloc du computingbuffer
		*computingbuffer = (fractal *) malloc(computeSize*sizeof(fractal));
		if(computingbuffer == NULL)
		{
			error(-1,0, "computingbuffer_malloc");
		}

	//initialisation des protecteurs d'acces a computingbuffer
	int computeMutexErr = pthread_mutex_init(&computebufferMutex, NULL);
	if(computeMutexErr != 0)
    {
        error(computeMutexErr,0, "mutex_init_compute");
    }
	int computeSemEmptyErr = sem_init(&computebufferEmpty, 0, computeSize);
	if(computeSemEmptyErr != 0)
    {
			error(computeSemEmptyErr,0, "sem_empty_init_compute");
    }
	int computeSemFullErr = sem_init(&computebufferFull, 0, 0);
	if(computeSemFullErr != 0)
    {
			error(computeSemFullErr,0, "sem_full_init_compute");
    }

	//lecture de readbuffer (= threads consommateurs) et remplissage du computingbuffer (= threads producteurs)
	pthread_t computeThreads[computeSize];
	int computeerr;
	long k;
	for (k = 0; k < computeSize; k++) {
		computeerr = pthread_create(&(computeThreads[k]), NULL,(void*) &thread_compute, (void *) &computeSize);
			if (computeerr != 0) {
				error(computeerr,0, "pthread_create_compute");
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
	  * 4eme etape : si l option -d est presente, on affiche la fractale maximale et on la met dans le fichier
		* nomme fichierOut (qui est le dernier element de argv) ou, si l option -d est presente, on affiche toutes
		* les fractales et on les met chacune dans un fichier portant le nom de la fractale.
	  */

    if (dflag != 1)
    {
        pthread_t compareThread;
        int compareErr = pthread_create(&(compareThread), NULL,(void *) &thread_compare, (void *) &computeSize);
        if (compareErr != 0) {
            error(compareErr,0, "pthread_create_compare");
        }
        write_bitmap_sdl(max_frac, fractal_get_name(max_frac));
				fractal_free(max_frac);
    }
    else{
        pthread_t displaythread;
        int displayErr = pthread_create(&(displaythread), NULL,(void *) &thread_compute_get, (void *) &computeSize);
        if (displayErr != 0)
				{
            error(displayErr,0, "pthread_create_display");
      	}

        fractal *frac;
        int join_displayErr = pthread_join(displaythread, (void **) &frac);
        if (join_displayErr != 0)
        {
            error(join_displayErr,0, "pthread_join_display");
        }
        write_bitmap_sdl(frac, fractal_get_name(frac));
    }

    //elimination des variables de protection des 2 buffers
    int err_destroy = sem_destroy(&readbufferEmpty);
    if (err_destroy != 0){
            error(err_destroy,0, "sem_destroy_err");
        }
    err_destroy = sem_destroy(&readbufferFull);
    if (err_destroy != 0){
            error(err_destroy,0, "sem_destroy_err");
        }
    err_destroy = sem_destroy(&computebufferEmpty);
    if (err_destroy != 0){
            error(err_destroy,0, "sem_destroy_err");
        }
    err_destroy = sem_destroy(&computebufferFull);
    if (err_destroy != 0){
            error(err_destroy,0, "sem_destroy_err");
        }

    int err_destroy_mutex = pthread_mutex_destroy(&readbufferMutex);
    if (err_destroy_mutex != 0){
            error(err_destroy_mutex,0, "mutex_destroy_err");
        }
    int err_destroy_mutex2 = pthread_mutex_destroy(&computebufferMutex);
    if (err_destroy_mutex2 != 0){
            error(err_destroy_mutex2,0, "mutex_destroy_err");
        }

		int z;
	  for(z = 0; z < count; z++)
	  {
		  free(name_list[z]);
	  }
	  free(name_list);

		int w;
		for(w = 0; w < readSize; w++)
		{
			free(readbuffer[w]);
		}
    free(readbuffer);

		int v;
		for(v = 0; v < computeSize; v++)
		{
			free(computingbuffer[v]);
		}
	  free(computingbuffer);

	  free(max_frac);

	  return 0;
}




/* -------------------------- FONCTIONS EXTERNES -------------------------- */


/*
* read
*
* Lit le fichier passe en argument, extrait chaque ligne et appelle la fonction extract
* avec la ligne courante en argument. Stocke ensuite la fractale retournee dans le buffer
* s il y a encore assez de place, attend que la place se libere sinon.
*
* @filename: nom du fichier dont on extrait les lignes.
* @return: 0 si on finit la lecture du fichier sans accroc, -1 autrement.
*/
void *thread_read(char* filename){
	FILE* fichier = NULL;

	fichier = fopen(filename, "r"); // on ouvre le fichier en lecture ("r" = read)
	char line[100];

	if (fichier != NULL)
	{
		while (fgets(line, 100, fichier) != NULL) // on lit le fichier tant qu on ne reeoit pas d erreur (NULL) donc tant qu il y a encore des lignes
		{
			fractal *fract = line_extract(line);

			// On met la fractale extraite dans le computingbuffer ou elle va attendre son calcul
			if (fract != NULL)
			{
				int i;
				int found = 0;
				sem_wait(&readbufferEmpty); //attente d'un slot libre dans readbuffer
				pthread_mutex_lock(&readbufferMutex);
				//section critique
					int black_listed = find_name(fract, name_list);
					if (black_listed == 1) {
						error(black_listed, 0, "nom_utilise");
					}
					name_list[count] = (char *) malloc(sizeof(fractal_get_name(fract)));
					strcpy(*(name_list + count),fractal_get_name(fract));
					count++;

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
	return NULL;
}

/*
 * extract
 *
 * Lit la cha�ne de caract�res extraite d'un fichier et extrait les informations pour les
 * stocker dans une structure fractale. null est renvoy� si le format "nom largeur hauteur
 * partie_r�elle partie_imaginaire" n'est pas respect�.
 *
 * @line: cha�ne de caract�res dont on doit extraire l'information
 * @return: une structure fractal contenant les informations ou null si une erreur survient.
 */
fractal *line_extract(char* line)
{
	int iter = 0;
	char *token;
	fractal *fract = fractal_new(NULL, 0, 0, 0.0, 0.0);

	// On obtient le premier mot de la ligne.
	token = strtok(line, " ");

	// On ignore les lignes invalides.
	if (token[0] == '#' || token == NULL)
	{
		return NULL;
	}
	char *copy = NULL;
	strcpy(copy,token);

	// On recupere tous les mots de la ligne qu'on stocke dans la structure fractal.
	while(token != NULL) {
		switch (iter) {
		case 0:
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
 * Lit le buffer de lecture, s il n'est pas vide, prend une fractale qui y est stockee, calcule
 * la valeur de chacun de ses pixels et la moyenne des valeurs de tous les pixels puis, place la
 * structure fractale ainsi modifiee dans le buffer de calcul des que celui-ci dispose d'une place
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

	int w = fractal_get_width(fcopy);
	int h = fractal_get_height(fcopy);
	int x;
	int y;

	for (x = 0; x < w; x++) {
		for (y = 0; y < h; y++) {
			int val = fractal_compute_value(fcopy, x, y);
			fractal_set_value(fcopy, x, y, val);
		}
	}

	fcopy->average = fractal_average(fcopy);

    int j;
    int foundBis = 0;
	sem_wait(&computebufferEmpty); //attente d'un slot libre dans computingbuffer.
    pthread_mutex_lock(&computebufferMutex);
    //section critique
    for(j=0; j<computeSize && foundBis == 0; j++)
    {
        if(computingbuffer[j] == NULL)
        {
            computingbuffer[j] = fcopy;
            foundBis = 1;
        }
    }
    //fin de section critique
    pthread_mutex_unlock(&computebufferMutex);
    sem_post(&computebufferFull);
		return NULL;
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
double fractal_average(const struct fractal *f) {
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
	return (sum / (w*h));
}

/*
 * thread_compare
 *
 * Lit le buffer de calcul, s il n'est pas vide, prend une fractale qui y est stockee, regarde si la
 * moyenne des valeurs de tous ses pixels est plus grande que la moyenne maximale actuelle, si c'est
 * le cas, remplace la fractale maximale par la fractale courante.
 *
 * @return: void
 */
void *thread_compare(int computeSize) {
    struct fractal *fcopy;
    int i;
    int found = 0;
	sem_wait(&computebufferFull); //attente d'un slot rempli dans computingbuffer
	pthread_mutex_lock(&computebufferMutex);
    //section critique
	for(i=0; i<computeSize && found == 0; i++)
    {
        if(computingbuffer[i] != NULL)
        {
            fcopy = computingbuffer[i];
            computingbuffer[i] = NULL;
            found = 1;
        }
    }
	//fin de section critique
  pthread_mutex_unlock(&computebufferMutex);
  sem_post(&computebufferEmpty);

	if (fcopy->average > max_frac->average) {
		max_frac = fcopy;
	}
	else {
		fractal_free(fcopy);
	}
	return NULL;
}

/*
 * thread_compute_get
 *
 * Lit le buffer de calcul, s il n'est pas vide, prend une fractale qui y est stockee et la retourne.
 *
 * @return: pointeur caste de la structure fractale lue.
 */
void *thread_compute_get(int computeSize)
{
    struct fractal *fcopy;
    int i;
    int found = 0;
	sem_wait(&computebufferFull); //attente d'un slot rempli dans computingbuffer
	pthread_mutex_lock(&computebufferMutex);
    //section critique
	for(i=0; i<computeSize && found == 0; i++)
    {
        if(computingbuffer[i] != NULL)
        {
            fcopy = computingbuffer[i];
            computingbuffer[i] = NULL;
            found = 1;
        }
    }
	//fin de section critique
    pthread_mutex_unlock(&computebufferMutex);
    sem_post(&computebufferEmpty);

    return ((void *) fcopy);
}

/*
 * find_name
 *
 * Compare la fractale avec un tableau contenant tous les noms des fractales déjà rencontrees. On
 * regarde si le nom de la fractale a deja ete utilise.
 *
 * @return: 1 si le nom est deja utilisé, 0 sinon.
 */
int find_name(struct fractal *f, char *namelist[64]) {
	int i;
	for(i=0;i<64;i++){
			if (strcmp(fractal_get_name(f), *(namelist + i))==0) {
				return 1;
			}
	}
	return 0;
}
