#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/file.h>

// Structure pour les messages échangés entre les processus
struct message {
    long message_type;
    int pid;
    int film_id;
    int age;
};

// Structure pour représenter une salle
typedef struct {
    int salle_id;
    int nb_places;
    int nb_places_libres;
    int film_id;
    int age_limite;
    int *client_pid;
} Salle;

// Enumération pour les différents statuts de réservation
typedef enum {
    RESERVATION_OK,
    SALLE_PLEINE,
    AGE_LIMITE
} ReservationStatus;

// Prototypes des fonctions
Salle create_salle(int salle_id, int nb_places, int film_id, int age_limite);
void recevoir_message(Salle salle[]);
void envoyer_confirmation_reservation(pid_t client_pid, int salle_id, ReservationStatus status);
void envoyer_signal_avec_cle(pid_t client_pid, int cle_salle, int type_evenement);
void salle_process(Salle salle);
void log_action(const char *message);
void reset_salle(Salle *salle);
void delete_salle(Salle *salle);

int main() {
    // Création de salles
    Salle salles[4];
    salles[0] = create_salle(1, 20, 1, 18);
    salles[1] = create_salle(2, 20, 2, 12);
    salles[2] = create_salle(3, 20, 3, 8);
    salles[3] = create_salle(4, 20, 4, 18);
    printf("Salles créées\n");

    // Création de processus pour chaque salle
    for (int i = 0; i < 4; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            salle_process(salles[i]);
            exit(0);
        }
    }

    // Simulation de réception de message
    recevoir_message(salles);

    // Attendre la fin des processus enfants
    for (int i = 0; i < 4; i++) {
        wait(NULL);
    }

    // Libérer la mémoire allouée pour les salles
    for (int i = 0; i < 4; i++) {
        delete_salle(&salles[i]);
    }
    return 0;
}

// Fonction pour créer une salle
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

// Fonction pour recevoir un message d'une file de messages
void recevoir_message(Salle salles[]) {
    int msgid, flag;
    key_t key1 = 17;
    flag = IPC_CREAT | 0666;
    struct message msg;

    // Création de la file de messages
    msgid = msgget(key1, flag);
    if (msgid < 0) {
        perror("Erreur lors de la création de la file de messages");
        exit(1);
    }

    while (true) {
        // Réception du message
        if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 2, MSG_NOERROR) < 0) {
            perror("Erreur lors de la réception du message");
            printf("Erreur lors de la réception du message\n");
            continue; // Continue to the next message
        }
        printf("message reçu par le client %d\n", msg.pid);

        char log_msg[100];
        for (int i = 0; i < 4; i++) {
            if (msg.film_id == salles[i].film_id) {
                if (msg.age < salles[i].age_limite) {
                    // Si le client est trop jeune, envoyer une confirmation avec un code d'erreur
                    snprintf(log_msg, sizeof(log_msg), "Le client %d est trop jeune pour le film\n", msg.pid);
                    printf("Le client %d est trop jeune pour le film\n", msg.pid);
                    log_action(log_msg);
                    envoyer_confirmation_reservation(msg.pid, salles[i].salle_id, AGE_LIMITE);
                } else if (salles[i].nb_places_libres > 0) {
                    // Si la salle a des places libres, réserver une place pour le client
                    salles[i].nb_places_libres--;
                    salles[i].client_pid[salles[i].nb_places_libres] = msg.pid;
                    snprintf(log_msg, sizeof(log_msg), "Client %d a réservé une place dans la salle %d\n", msg.pid, salles[i].salle_id);
                    printf("Client %d a réservé une place dans la salle %d\n", msg.pid, salles[i].salle_id);
                    log_action(log_msg);
                    envoyer_confirmation_reservation(msg.pid, salles[i].salle_id, RESERVATION_OK);
                } else {
                    // Si la salle est pleine, envoyer une confirmation avec un code d'erreur
                    snprintf(log_msg, sizeof(log_msg), "La salle %d est pleine\n", salles[i].salle_id);
                    printf("La salle %d est pleine\n", salles[i].salle_id);
                    log_action(log_msg);
                    envoyer_confirmation_reservation(msg.pid, salles[i].salle_id, SALLE_PLEINE);
                }
                break;
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

    // Envoi du signal
    if (sigqueue(client_pid, SIGUSR1, valeur) == -1) {
        perror("Erreur lors de l'envoi du signal");
    }
    printf("Signal envoyé au client %d\n", client_pid);
}

// Fonction pour envoyer un signal à un client
void envoyer_signal_avec_cle(pid_t client_pid, int cle_salle, int type_evenement) {
    union sigval valeur;
    // Encodage : combine type d'événement et clé dans un seul entier
    valeur.sival_int = (type_evenement << 16) | (cle_salle & 0xFFFF);
    printf("Envoi du signal avec clé %d et type d'événement %d\n", cle_salle, type_evenement);

    if (sigqueue(client_pid, SIGUSR2, valeur) == -1) {
        perror("Erreur lors de l'envoi du signal");
    } else {
        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "Signal envoyé avec clé %d et type d'événement %d\n", cle_salle, type_evenement);
        printf("Signal envoyé avec clé %d et type d'événement %d\n", cle_salle, type_evenement);
        log_action(log_msg);
    }
}

// Processus de chaque salle
void salle_process(Salle salle) {
    while (1) {
        printf("Attendre 10 secondes pour la prochaine projection dans la salle %d\n", salle.salle_id);
        sleep(30); // Attendre 3 minutes

        // Envoyer signal de début de projection
        for (int i = 0; i < salle.nb_places - salle.nb_places_libres; i++) {
            envoyer_signal_avec_cle(salle.client_pid[i], salle.salle_id, 1);
            printf("début de la projection de la salle %d\n", salle.salle_id);
        }

        sleep(30); // Durée de la projection

        // Envoyer signal de fin de projection
        for (int i = 0; i < salle.nb_places - salle.nb_places_libres; i++) {
            envoyer_signal_avec_cle(salle.client_pid[i], salle.salle_id, 2);
            printf("Fin de la projection de la salle %d\n", salle.salle_id);
        }

        // Réinitialiser la salle
        reset_salle(&salle);
    }
    free(salle.client_pid);
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

//Fonction pour supprimer une salle
void delete_salle(Salle *salle) {
    if (salle->client_pid != NULL) {
        free(salle->client_pid);
        salle->client_pid = NULL; // Assurez-vous que le pointeur ne pointe plus sur une zone mémoire libérée
    }
}

// Fonction pour enregistrer une action dans le fichier de log
void log_action(const char *message) {
    // Ouvrir le fichier de log en mode append
    FILE *log_file = fopen("log.txt", "a");
    if (log_file != NULL) {
        struct flock lock;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = 0;
        fcntl(fileno(log_file), F_SETLKW, &lock); // Verrouillez le fichier
        fprintf(log_file, "%s", message);
        lock.l_type = F_UNLCK;
        fcntl(fileno(log_file), F_SETLK, &lock); // Déverrouillez le fichier
        fclose(log_file);
    } else {
        perror("Erreur lors de l'ouverture du fichier de log");
    }
}
