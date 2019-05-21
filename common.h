#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <errno.h>
#include <stdbool.h>

#define MAXPARTICIPANTS 5
#define TAILLE_MSG 128
#define TAILLE_NOM 25
#define NB_LIGNES 20


void gen_socket(char* dest, char* base, int id) {
    snprintf(dest, TAILLE_NOM, "%s_%i", base, id);
    if (mkfifo(dest, S_IRUSR|S_IWUSR) < 0) {
        printf("Impossible de créer le pipe %s\n", dest);
        exit(3);
    }
}