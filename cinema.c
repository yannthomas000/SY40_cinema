#include "cinema_manager.h"
#include "salle.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

struct message {
    long message_type;
    int pid;
    int film_id;
};

typedef struct {
    int salle_id;
    int nb_places;
    int nb_places_libres;
    int film_id;
    int *client_pid;
} Salle;

Salle create_salle(int salle_id, int nb_places, int film_id);
void recevoir_message();
void envoyer_confirmation_reservation(pid_t client_pid, int salle_id);

int main() {
    // Création de salles
    Salle salles[4];
    salles[0] = create_salle(1, 100, 101);
    salles[1] = create_salle(2, 150, 102);
    salles[2] = create_salle(3, 200, 103);
    salles[3] = create_salle(4, 250, 104);

    // Affichage des informations des salles
    for (int i = 0; i < 4; i++) {
        printf("Salle ID: %d, Places: %d, Places Libres: %d, Film ID: %d\n",
               salles[i].salle_id,
               salles[i].nb_places,
               salles[i].nb_places_libres,
               salles[i].film_id);
    }

    // Simulation de réception de message
    recevoir_message();

    // Simulation d'envoi de confirmation de réservation
    envoyer_confirmation_reservation(salles[0].client_pid[0], salles[0].salle_id);

    return 0;
}

// Fonction pour recevoir un message d'une file de messages
void recevoir_message() {
    int msgid, flag;
    key_t key1 = 17;
    flag = IPC_CREAT | 0666;
    struct message msg;

    msgid = msgget(key1, flag);

    msgrcv(msgid, &msg, sizeof(msg), 2, MSG_NOERROR);
}

// Fonction pour envoyer une confirmation de réservation à un client
void envoyer_confirmation_reservation(pid_t client_pid, int salle_id) {
    union sigval valeur;
    valeur.sival_int = salle_id; // Envoi de l'ID de réservation
    if (sigqueue(client_pid, SIGUSR1, valeur) == -1) {
        perror("Erreur lors de l'envoi du signal");
    }
}

Salle create_salle(int salle_id, int nb_places, int film_id) {
    Salle salle;
    salle.salle_id = salle_id;
    salle.nb_places = nb_places;
    salle.nb_places_libres = nb_places;
    salle.film_id = film_id;
    salle.client_pid = malloc(nb_places * sizeof(int));
    return salle;
}

// Fonction pour envoyer un message de début de séance à un client
void envoyer_signal_avec_cle(pid_t client_pid, int cle_salle, int type_evenement) {
    union sigval valeur;
    // Encodage : combiner type d'événement et clé dans un seul entier
    valeur.sival_int = (type_evenement << 16) | (cle_salle & 0xFFFF);

    if (sigqueue(client_pid, SIGUSR2, valeur) == -1) {
        perror("Erreur lors de l'envoi du signal");
    } else {
        printf("Signal envoyé avec clé %d et type d'événement %d\n", cle_salle, type_evenement);
    }
}
