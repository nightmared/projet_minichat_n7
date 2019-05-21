#include "common.h"

#include <curses.h>
#include <sys/time.h>

static char discussion[NB_LIGNES][TAILLE_MSG]; /* derniers messages reçus */
static int pos = 0;
static int s2c_listener, c2s_writer;

void afficher() { 
    clear();
    //refresh();
    printf("==============================(discussion)==============================\n");
    for (int i=0; i<NB_LIGNES; i++) {
        printf("%s\n", discussion[(pos+i)%NB_LIGNES]);
    }
    printf("------------------------------------------------------------------------\n");
}

bool handle_input(char* input, bool from_stdin) {
    if (strcmp(input, "fin\n") == 0)
        return false;

    pos = (pos+1)%NB_LIGNES;
    strncpy(discussion[pos], input, TAILLE_MSG);
    afficher();

    if (from_stdin) {
        write(c2s_writer, input, TAILLE_MSG*sizeof(char));
    }

    return true;
}

int main (int argc, char *argv[]) {
    // seed the random generator so that no two seed will be equal (between clients)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);

    if (!((argc == 2) && (strlen(argv[1]) < TAILLE_NOM*sizeof(char)))) {
        printf("utilisation : %s <pseudo>\n", argv[0]);
        printf("Le pseudo ne doit pas dépasser 24 caractères\n");
        exit(1);
    }

    int fifo_writer;
    if ((fifo_writer = open("./ecoute",O_WRONLY)) == -1) {
        printf("Le serveur doit être lance, et depuis le meme repertoire que le client\n");
        exit(2);
    }

    
    // on génère les noms de fichiers
    char names_s2c[TAILLE_NOM];
    char names_c2s[TAILLE_NOM];
    int id = rand();
    gen_socket(names_s2c, "s2c", id);
    gen_socket(names_c2s, "c2s", id);

    // on envoie les identifiants de fichier au server
    write(fifo_writer, &id, sizeof(id));

    // on ouvre les fifo
    s2c_listener = open(names_s2c, O_RDONLY);
    c2s_writer = open(names_c2s, O_WRONLY);

    // on envoie le pseudo sur notre "lien dédié"
    write(c2s_writer, argv[2], strlen(argv[2])+1);

    char buf[TAILLE_MSG];
    bool cont = true;
    do {
        fd_set listeners;
        FD_ZERO(&listeners);
        FD_SET(STDIN_FILENO, &listeners);
        FD_SET(s2c_listener, &listeners);

        // attente d'entrées utilisateur
        int modif = select(2, &listeners, NULL, NULL, NULL);
        if (modif == -1) {
            // prise en charge des signaux
            if (errno == EINTR)
                continue;
            else
                break;
        }

        if (FD_ISSET(s2c_listener, &listeners)) {
            int nb_read = read(s2c_listener, buf, (TAILLE_MSG-1)*sizeof(char));
            buf[nb_read] = 0;
            cont &= handle_input(buf, false);
        }
        if (FD_ISSET(s2c_listener, &listeners)) {
            int nb_read = read(STDIN_FILENO, buf, (TAILLE_MSG-1)*sizeof(char));
            buf[nb_read] = 0;
            cont &= handle_input(buf, true);
        }
    } while (cont);

    // fermeture des fichiers
    close(c2s_writer);
    unlink(names_c2s);
    close(s2c_listener);
    unlink(names_s2c);
    printf("fin client\n");
    exit (0);
}
