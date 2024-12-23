#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>

struct message {
    long message_type;
    int pid;
    int film_id;
    int age;
};

typedef struct {
    int salle_id;
    int nb_places;
    int nb_places_libres;
    int film_id;
    int age_limite;
    int *client_pid;
} Salle;

typedef enum {
    RESERVATION_OK,
    SALLE_PLEINE,
    AGE_LIMITE
} ReservationStatus;

Salle create_salle(int salle_id, int nb_places, int film_id, int age_limite);
void recevoir_message(Salle salle[]);
void envoyer_confirmation_reservation(pid_t client_pid, int salle_id, ReservationStatus status);
void envoyer_signal_avec_cle(pid_t client_pid, int cle_salle, int type_evenement);
void salle_process(Salle salle);
void log_action(const char *message);
void reset_salle(Salle *salle);

int main() {
    // Création de salles
    Salle salles[4];
    salles[0] = create_salle(1, 100, 101, 18);
    salles[1] = create_salle(2, 150, 102, 12);
    salles[2] = create_salle(3, 200, 103, 8);
    salles[3] = create_salle(4, 250, 104, 18);

    // Forking processes for each salle
    for (int i = 0; i < 4; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            salle_process(salles[i]);
            exit(0);
        }
    }

    // Simulation de réception de message
    recevoir_message(salles);

    // Wait for all child processes to finish
    for (int i = 0; i < 4; i++) {
        wait(NULL);
    }

    return 0;
}

// Fonction pour recevoir un message d'une file de messages
void recevoir_message(Salle salles[]) {
    int msgid, flag;
    key_t key1 = 17;
    flag = IPC_CREAT | 0666;
    struct message msg;

    msgid = msgget(key1, flag);

    msgrcv(msgid, &msg, sizeof(msg), 2, MSG_NOERROR);

    char log_msg[100];
    //recherche dans les salles disponibles celle ou le film est projete
    for (int i = 0; i < 4; i++) {
        if (msg.film_id == salles[i].film_id) {
            // Si la salle a des places libres
            if (msg.age < salles[i].age_limite) {
                snprintf(log_msg, sizeof(log_msg), "Le client %d est trop jeune pour le film\n", msg.pid);
                log_action(log_msg);
                envoyer_confirmation_reservation(msg.pid, salles[i].salle_id, AGE_LIMITE);
            } else if (salles[i].nb_places_libres > 0) {
                // Réserver une place
                salles[i].nb_places_libres--;
                salles[i].client_pid[salles[i].nb_places_libres] = msg.pid;
                snprintf(log_msg, sizeof(log_msg), "Client %d a réservé une place dans la salle %d\n", msg.pid, salles[i].salle_id);
                log_action(log_msg);
                envoyer_confirmation_reservation(msg.pid, salles[i].salle_id, RESERVATION_OK);
            } else {
                snprintf(log_msg, sizeof(log_msg), "La salle %d est pleine\n", salles[i].salle_id);
                log_action(log_msg);
                envoyer_confirmation_reservation(msg.pid, salles[i].salle_id, SALLE_PLEINE);
            }
        }
    }
}

// Fonction pour envoyer une confirmation de réservation à un client
void envoyer_confirmation_reservation(pid_t client_pid, int salle_id, ReservationStatus status) {
    union sigval valeur;
    if (status == AGE_LIMITE) {
        valeur.sival_int = 999; // Envoi d'une erreur
    } else if(status == SALLE_PLEINE) {
        valeur.sival_int = 888; // Envoi d'une erreur
    } else {
        valeur.sival_int = salle_id; // Envoi de l'ID de réservation
    }

    if (sigqueue(client_pid, SIGUSR1, valeur) == -1) {
        perror("Erreur lors de l'envoi du signal");
    }
}

Salle create_salle(int salle_id, int nb_places, int film_id, int age_limite) {
    Salle salle;
    salle.salle_id = salle_id;
    salle.nb_places = nb_places;
    salle.nb_places_libres = nb_places;
    salle.film_id = film_id;
    salle.age_limite = age_limite;
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
        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "Signal envoyé avec clé %d et type d'événement %d\n", cle_salle, type_evenement);
        log_action(log_msg);
    }
}

// Processus de chaque salle
void salle_process(Salle salle) {
    while (1) {
        sleep(180); // Attendre 3 minutes

        // Envoyer signal de début de projection
        for (int i = 0; i < salle.nb_places - salle.nb_places_libres; i++) {
            envoyer_signal_avec_cle(salle.client_pid[i], salle.salle_id, 1);
        }

        sleep(30); // Durée de la projection

        // Envoyer signal de fin de projection
        for (int i = 0; i < salle.nb_places - salle.nb_places_libres; i++) {
            envoyer_signal_avec_cle(salle.client_pid[i], salle.salle_id, 2);
        }

        // Réinitialiser la salle
        reset_salle(&salle);
    }
}

// Fonction pour réinitialiser une salle
void reset_salle(Salle *salle) {
    salle->nb_places_libres = salle->nb_places;
    for (int i = 0; i < salle->nb_places; i++) {
        salle->client_pid[i] = 0;
    }
    char log_msg[100];
    snprintf(log_msg, sizeof(log_msg), "Salle %d réinitialisée\n", salle->salle_id);
    log_action(log_msg);
}

// Fonction pour enregistrer les actions dans un fichier de log
void log_action(const char *message) {
    FILE *log_file = fopen("log.txt", "a");
    if (log_file != NULL) {
        fprintf(log_file, "%s", message);
        fclose(log_file);
    } else {
        perror("Erreur lors de l'ouverture du fichier de log");
    }
}
