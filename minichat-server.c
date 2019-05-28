#include "common.h"

struct client {                     /* descripteur de participant */
    bool actif;
    char nom [TAILLE_NOM];
    int in;        /* tube d'entrée (C2S) */
    int out;    /* tube de sortie (S2C) */
};


static struct client clients[MAXPARTICIPANTS];
static int listener;
static char buf[TAILLE_MSG];

void reset(int i) {
    clients[i].actif = false;
    memset(clients[i].nom, 0, TAILLE_NOM*sizeof(char));
    clients[i].in = -1;
    clients[i].out = -1;
}

void diffuser(int origin) {
    for (int i=0; i < MAXPARTICIPANTS; i++) {
        if (clients[i].actif && i != origin) {
            char nom[TAILLE_NOM];
            int max_pseudo_len = MIN(strlen(clients[i].nom), TAILLE_NOM-4);
            snprintf(nom, max_pseudo_len+2, "<%s", clients[i].nom);
            snprintf(nom+max_pseudo_len+1, TAILLE_NOM-max_pseudo_len, ">%*c", TAILLE_NOM-max_pseudo_len-3, ' ');
            nom[TAILLE_NOM-1] = ' ';
            write(clients[i].out, nom, TAILLE_NOM*sizeof(char));
            write(clients[i].out, buf, (TAILLE_MSG-TAILLE_NOM)*sizeof(char));
        }
    }
}

void cleanup() {
    printf("On m'a demandé de quitter, au revoir!\n");
    for (int i=0; i < MAXPARTICIPANTS; i++) {
        if (clients[i].actif) {
            close(clients[i].in);
            close(clients[i].out);
        }
    }
    close(listener);
    unlink("ecoute");
    exit(1);
}

int main (int argc, char *argv[]) {
    for (int i=0; i < MAXPARTICIPANTS; i++) {
        reset(i);
    }
        
    // suppression du précédent pipe, s'il existe encore
    unlink("ecoute");
    int res = mkfifo("ecoute", S_IRUSR|S_IWUSR);
    if (res == -1) {
        perror("Couldn't open the pipe");
        exit(1);
    }
    listener = open("ecoute", O_RDONLY | O_NONBLOCK);
    // descripteur de fichier nécessaire pour empécher le serveur de se fermer en l'absence de clients
    open("ecoute", O_WRONLY);

    while (true) {
        // création de la liste des descripteurs à vérifier
        fd_set polled_fds;
        FD_ZERO(&polled_fds);
        FD_SET(listener, &polled_fds);
        int max_fd = listener;
        for (int i=0; i < MAXPARTICIPANTS; i++) {
            if (clients[i].actif) {
                FD_SET(clients[i].in, &polled_fds);
                if (clients[i].in > max_fd)
                    max_fd = clients[i].in;
            }
        }

        // on attends
        int modif = select(max_fd+1, &polled_fds, NULL, NULL, NULL);
        if (modif == -1) {
            // prise en charge des signaux
            if (errno == EINTR)
                continue;
            else
                break;
        }
        
        for (int i=0; i < MAXPARTICIPANTS; i++) {
            if (clients[i].actif && FD_ISSET(clients[i].in, &polled_fds)) {
                int nb_read = read(clients[i].in, buf, (TAILLE_MSG-TAILLE_NOM)*sizeof(char));
                if (nb_read <= 0) {
                    // on déconnecte ce client
                    printf("L'utilisateur %s s'est déconnecté\n", clients[i].nom);
                    close(clients[i].in);
                    close(clients[i].out);
                    reset(i);
                    continue;
                }
                buf[nb_read] = 0;
                diffuser(i);
            }
        }

        // Oh, un nouveau client !
        if (FD_ISSET(listener, &polled_fds)) {
            for (int i=0; i < MAXPARTICIPANTS; i++) {
                if (!clients[i].actif) {
                    clients[i].actif = true;
                    int id;
                    read(listener, &id, sizeof(int));
                    char name[TAILLE_NOM];
                    gen_socket_name(name, "s2c", id);
                    clients[i].out = open(name, O_WRONLY);
                    gen_socket_name(name, "c2s", id);
                    clients[i].in = open(name, O_RDONLY);
                    clients[i].nom[read(clients[i].in, clients[i].nom, TAILLE_NOM*sizeof(char))] = 0;
                    if (strlen(clients[i].nom) == 3 && !strncmp(clients[i].nom, "fin", 3))
                        cleanup();
                    printf("Nouveau client connecté: %s\n", clients[i].nom);
                    break;
                }
            }

        }
    }
}
