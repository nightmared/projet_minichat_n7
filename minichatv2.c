#include "common.h"

#include <sys/time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/mman.h>

struct message {
    int numero;
    char auteur [TAILLE_NOM];
    char texte [TAILLE_MSG];
};

static int pos = 0, tty;
static struct message *discussion;
static bool cont = true;
static char* my_pseudo = NULL;

int window_size() {
    struct winsize ws;
    if (ioctl(tty, TIOCGWINSZ, &ws) < 0) {
        return 15;
    }
    return ws.ws_row;
}

// On récupères le message décale de offset par rapport à al aposition actuelle
struct message* get_msg(int offset) {
    int cur = (pos+offset)%NB_LIGNES;
    if (cur < 0)
        cur = NB_LIGNES-1-cur;
    return (struct message*)(discussion+cur);
}

void afficher() { 
    // On efface l'écran
    write(1, "\033[H""\033[J", 6);

    printf("==============================(discussion)==============================\n");
    for (int i=MIN(window_size()-3, pos)-1; i>=0; i--) {
        struct message *m = get_msg(-i);
        char nom[TAILLE_NOM];
        int max_pseudo_len = MIN(strlen(m->auteur), TAILLE_NOM-4);
        snprintf(nom, max_pseudo_len+2, "<%s", m->auteur);
        snprintf(nom+max_pseudo_len+1, TAILLE_NOM-max_pseudo_len, ">%*c", TAILLE_NOM-max_pseudo_len-3, ' ');
        printf("%s %s\n", nom, m->texte);
    }
    printf("------------------------------------------------------------------------\n");
}

bool handle_input(char *input) {
    pos = pos+1;
    struct message *new_message = get_msg(0);

	if (input != NULL) {
        // on élimine le '\n' en fin de ligne
        input[strlen(input)-1] = '\0';
		// fin du client ?
		if (strlen(input) == 3 && strcmp(input, "fin") == 0)
			return false;

		struct message *current_message = get_msg(-1);
        memcpy(new_message->auteur, my_pseudo, TAILLE_NOM);
        memcpy(new_message->texte, input, TAILLE_MSG);
		new_message->numero = current_message->numero+1;
	}

    afficher();

    return true;
}

int main (int argc, char *argv[]) {
    tty = open("/dev/tty", O_RDWR);

    if (!((argc == 2) && (strlen(argv[1]) < TAILLE_NOM*sizeof(char)))) {
        printf("utilisation : %s <pseudo>\n", argv[0]);
        printf("Le pseudo ne doit pas dépasser 24 caractères\n");
        exit(1);
    }
    my_pseudo = argv[1];

    // on active le mode nonbloquant pour la lecture sur l'entrée standard
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL)|O_NONBLOCK);

	// il est temps de mmaper notre fichier
	int fd = open("fd_echange_v2", O_CREAT|O_TRUNC|O_RDWR, 0644);
    // on alloue une page par excédent
	int nb_pages = ((NB_LIGNES*sizeof(struct message))/sysconf(_SC_PAGE_SIZE)+1)*sysconf(_SC_PAGE_SIZE);
	// on préalloue la place dans le fichier
	ftruncate(fd, nb_pages);
	discussion = mmap(NULL, nb_pages, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    afficher();

    char buf[TAILLE_MSG];
    do {
        int nb_read = read(STDIN_FILENO, buf, (TAILLE_MSG)*sizeof(char));
        if (nb_read > 0)
			cont &= handle_input(buf);
		if (get_msg(1)->numero > get_msg(0)->numero)
			handle_input(NULL);
		sleep(1);
    } while (cont);

    // fermeture des fichiers
    printf("fin client\n");
    exit (0);
}
