#include "common.h"

#include <curses.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <signal.h>

static char discussion[MAXPARTICIPANTS*2][TAILLE_MSG]; /* derniers messages reçus */
static int pos = 0;
static bool cont = true;
static int s2c_listener, c2s_writer;
static int tty;

int window_size() {
    struct winsize ws;
    if (ioctl(tty, TIOCGWINSZ, &ws) < 0) {
        return 15;
    }
    return ws.ws_row;
}

// On récupères le message décale de offset par rapport à al aposition actuelle
char* get_msg(int offset) {
    int cur = (pos+offset)%(MAXPARTICIPANTS*2);
    if (cur < 0)
        cur = MAXPARTICIPANTS-1-cur;
    return discussion[cur];
}

void afficher() { 
    // On efface l'écran
    write(1, "\033[H""\033[J", 6);

    printf("==============================(discussion)==============================\n");
    for (int i=MIN(window_size()-3, MAXPARTICIPANTS*2)-1; i>=0; i--) {
        printf("%s\n", get_msg(-i));
    }
    printf("------------------------------------------------------------------------\n");
}

void handle_sigint(int sig) {
    cont = false;
}

bool handle_input(char* input, bool from_stdin) {
    if (strcmp(input, "fin\n") == 0)
        return false;

    pos = (pos+1)%(MAXPARTICIPANTS*2);

    if (from_stdin) {
        // on élimine le '\n' en fin de ligne
        input[strlen(input)-1] = '\0';
        write(c2s_writer, input, (TAILLE_MSG-TAILLE_NOM)*sizeof(char));
        sprintf(discussion[pos], "<me>%*c", TAILLE_NOM-4, ' ');
        strncpy(discussion[pos]+TAILLE_NOM, input, TAILLE_MSG-TAILLE_NOM);
    } else {
        strncpy(discussion[pos], input, TAILLE_MSG);
    }
    afficher();

    return true;
}

int main (int argc, char *argv[]) {
    tty = open("/dev/tty", O_RDWR);
    signal(SIGINT, handle_sigint);

    // seed the random generator so that no two seed will be equal (between clients)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);

    if (!((argc == 2) && (strlen(argv[1]) < TAILLE_NOM*sizeof(char)))) {
        printf("utilisation : %s <pseudo>\n", argv[0]);
        printf("Le pseudo ne doit pas dépasser 24 caractères\n");
        exit(1);
    }

    int fifo_writer = open("ecoute",O_WRONLY);
    if (fifo_writer == -1) {
        perror("Le serveur doit être lance, et depuis le meme repertoire que le client");
        exit(2);
    }

    
    // on génère les noms de fichiers
    char names_s2c[TAILLE_NOM];
    char names_c2s[TAILLE_NOM];
    int id = rand();
    gen_socket(names_s2c, "s2c", id);
    gen_socket(names_c2s, "c2s", id);

    // on envoie les identifiants de fichier au server
    write(fifo_writer, &id, sizeof(int));

    // on ouvre les fifo
    s2c_listener = open(names_s2c, O_RDONLY);
    c2s_writer = open(names_c2s, O_WRONLY);

    // on envoie le pseudo sur notre "lien dédié"
    write(c2s_writer, argv[1], strlen(argv[1])+1);

    // on initialise toute la discussion à zéro
    memset(discussion, '\0', MAXPARTICIPANTS*2*TAILLE_MSG);
    afficher();

    char buf[TAILLE_MSG];
    do {
        fd_set listeners;
        FD_ZERO(&listeners);
        FD_SET(STDIN_FILENO, &listeners);
        FD_SET(s2c_listener, &listeners);

        // attente d'entrées utilisateur
        int modif = select(MAX(STDIN_FILENO, s2c_listener)+1, &listeners, NULL, NULL, NULL);
        if (modif == -1) {
            // prise en charge des signaux
            if (errno == EINTR)
                continue;
            else
                break;
        }

        if (FD_ISSET(s2c_listener, &listeners)) {
            int nb_read = read(s2c_listener, buf, (TAILLE_MSG)*sizeof(char));
            if (nb_read <= 0) {
                // le serveur a quitté
                cont = false;
            }
            buf[nb_read] = 0;
            cont &= handle_input(buf, false);
        }
        if (FD_ISSET(STDIN_FILENO, &listeners)) {
            int nb_read = read(STDIN_FILENO, buf, (TAILLE_MSG-TAILLE_NOM)*sizeof(char));
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
