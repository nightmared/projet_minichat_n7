#include "common.h"

struct client { 					/* descripteur de participant */
    bool actif;
    char nom [TAILLE_NOM];
    int in;		/* tube d'entrée (C2S) */
    int out;	/* tube de sortie (S2C) */
};


static struct client clients[MAXPARTICIPANTS];

static char buf[TAILLE_MSG]; 	/* tampon de messages reçus/à rediffuser */
static int nbactifs = 0;

void effacer(int i) { /* efface le descripteur pour le participant i */
    clients[i].actif = false;
    memset(clients[i].nom, 0, TAILLE_NOM*sizeof(char));
    clients[i].in = -1;
    clients[i].out = -1;
}

void diffuser(char *dep, int origin) {
}

void desactiver (int p) {
/* traitement d'un participant déconnecté (à faire) */
}

int main (int argc, char *argv[]) {
    for (int i=0; i < MAXPARTICIPANTS; i++) {
        effacer(i);
    }
		
    mkfifo("./ecoute", S_IRUSR|S_IWUSR);
    int listener=open("./ecoute", O_RDONLY);
    // descripteur de fichier nécessaire pour empécher le serveur de se fermer en l'absence de clients
    open("./ecoute", O_WRONLY);

    while (true) {
        // création de la liste des descripteurs à vérifier
        fd_set polled_fds;
        FD_ZERO(&polled_fds);
        FD_SET(listener, &polled_fds);
        int fd_count = 1;
        for (int i=0; i < MAXPARTICIPANTS; i++) {
            if (clients[i].actif) {
                FD_SET(clients[i].in, &polled_fds);
                fd_count++;
            }
        }

        // on attends
        int modif = select(fd_count, &polled_fds, NULL, NULL, NULL);
        if (modif == -1) {
            // prise en charge des signaux
            if (errno == EINTR)
                continue;
            else
                break;
        }
        
        for (int i=0; i < MAXPARTICIPANTS; i++) {
            if (clients[i].actif && FD_ISSET(clients[i].in, &polled_fds)) {
                buf[read(clients[i].in, buf, (TAILLE_MSG-1)*sizeof(char))] = 0;



            }
        }
        // Oh, un nouveau client !
        if (FD_ISSET(listener, &polled_fds)) {
            for (int i=0; i < MAXPARTICIPANTS; i++) {
                if (!clients[i].actif) {
                    clients[i].actif = true;
                    int id;
                    read(listener, &id, sizeof(int));
                    char names_s2c[TAILLE_NOM];
                    char names_c2s[TAILLE_NOM];
                    gen_socket(names_s2c, "s2c", id);
                    gen_socket(names_c2s, "c2s", id);
                    clients[i].in = open(names_c2s, O_RDONLY);
                    clients[i].out = open(names_s2c, O_WRONLY);
                    clients[i].nom[read(clients[i].in, clients[i].nom, TAILLE_NOM*sizeof(char))] = 0;
                    break;
                }
            }

        }
        printf("participants actifs : %d\n",nbactifs);

		/* boucle du serveur : traiter les requêtes en attente 
				 * sur le tube d'écoute : lorsqu'il y a moins de MAXPARTICIPANTS actifs.
				 	ajouter de nouveaux participants et les tubes d'entrée.			  
				 * sur les tubes de service : lire les messages sur les tubes c2s, et les diffuser.
				   Note : tous les messages comportent TAILLE_MSG caractères, et les constantes
           sont fixées pour qu'il n'y ait pas de message tronqué, ce qui serait  pénible 
           à gérer. Enfin, on ne traite pas plus de TAILLE_RECEPTION/TAILLE_MSG*sizeof(char)
           à chaque fois.
           - dans le cas où la terminaison d'un participant est détectée, gérer sa déconnexion
			
			(à faire)
		*/
    }
}
