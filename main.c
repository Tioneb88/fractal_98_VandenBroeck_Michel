#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "fractal.h"

int main(int argc, char *argv[])
{
	/*
	 * 1ère étape : récupération des arguments et des options de la ligne de commande.
	 */

	// On sait que argv[0] contient le nom de l'exécutable,
	// que argv[argc-1] est le nom du fichier de sortie final
	// et que les argc-2 autres valeurs de argv (entre argv[0] et argv[argc-1]) contiennent les noms des fichiers (ou la ligne de commande) à lire.

	// On récupère les options et les arguments passés en ligne de commande.
	int dflag = 0;
	int maxthread = 1;
	int error;

	while ((c = getopt(argc, argv, "maxthread:d")) != -1)
		switch (c)
		{
		case 'd':
			dflag = 1;
			break;
		case 'maxthreads':
			maxthread = optarg;
			break;
		case '?':
			if (optopt == 'c')
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

	char *fileNames;

	/*
	* 2ème étape : on lance un thread de lecture par fichier ou pour l'entrée standard.
	*/

	pthread_t threads[argc];
	int err;

	for (int i = 0; i < argc; i++)
	{
		err = pthread_create(&threads[i], &read, (void *) &fileNames[i]);
		if (err != 0)
		{
			error(err, "pthread_create");
		}
	}



}
