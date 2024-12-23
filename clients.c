#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <string.h>

#define NUM_CLIENTS 5

struct message {
    long message_type;
    int pid;
    int film_id;
    int age;
};

typedef struct {
    int id;
    int priority;
    int age;
    char status[50];
    int film_id;
} Client;

Client create_client(int id, int priority, int age);
bool reserver_film(Client *client);
void gestionnaire_signal(int sig, siginfo_t *info, void *context);
void reset_client(Client *client);
void log_action(const char *message);
void client_process(Client client);

int main(void) {
    pid_t pids[NUM_CLIENTS];
    int i;

    // Enregistrement du gestionnaire de signal
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = gestionnaire_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    for (i = 0; i < NUM_CLIENTS; i++) {
        if ((pids[i] = fork()) < 0) {
            perror("fork");
            exit(1);
        } else if (pids[i] == 0) {
            // Code du processus enfant
            Client client = create_client(i + 1, rand() % 10, rand() % 100);
            client_process(client);
            exit(0);
        }
    }

    // Attente de la fin de tous les processus enfants
    for (i = 0; i < NUM_CLIENTS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    log_action("Tous les clients ont terminé leurs activités.\n");

    return 0;
}

void client_process(Client client) {
    char log_msg[100];
    while (1) {
        // Attendre entre 5 et 10 secondes
        sleep(5 + rand() % 6);

        // Réservation d'un film
        if (reserver_film(&client)) {
            snprintf(log_msg, sizeof(log_msg), "Client %d a réservé un film\n", client.id);
            log_action(log_msg);
        }

        // Attendre la confirmation de réservation
        pause();

        if (strcmp(client.status, "attend") == 0) {
            // Attendre le début du film
            pause();

            if (strcmp(client.status, "regarde un film") == 0) {
                // Attendre la fin du film
                pause();
            }
        }

        // Réinitialisation du client après la fin du film ou échec de réservation
        reset_client(&client);
        snprintf(log_msg, sizeof(log_msg), "Client %d réinitialisé\n", client.id);
        log_action(log_msg);
    }
}

// Création d'un nouveau client
Client create_client(int id, int priority, int age) {
    Client client;
    client.id = id;
    client.priority = priority;
    client.age = age;
    strcpy(client.status, "libre");
    client.film_id = -1;
    return client;
}

// Fonction pour réserver un film en envoyant un message dans une file de messages
bool reserver_film(Client *client) {
    int msgid, flag;
    key_t key1 = 17;
    flag = IPC_CREAT | 0666;
    struct message msg;

    msgid = msgget(key1, flag);

    msg.message_type = 2;
    msg.pid = getpid();
    msg.film_id = rand() % 3;
    msg.age = client->age;

    msgsnd(msgid, &msg, sizeof(msg), IPC_NOWAIT);

    return true;
}

void gestionnaire_signal(int sig, siginfo_t *info, void *context) {
    Client *client = (Client *)context;
    char log_msg[100];
    if (sig == SIGUSR1) {
        int film_id = info->si_value.sival_int;
        if (film_id == 999) {
            snprintf(log_msg, sizeof(log_msg), "Le client %d est trop jeune pour le film\n", client->id);
            log_action(log_msg);
            reset_client(client);
        } else if(film_id == 888){
            snprintf(log_msg, sizeof(log_msg), "La salle est pleine pour le client %d\n", client->id);
            log_action(log_msg);
            reset_client(client);
        }else{
            client->film_id = film_id;
            strcpy(client->status, "attend");
            snprintf(log_msg, sizeof(log_msg), "Client %d attend pour le film %d\n", client->id, film_id);
            log_action(log_msg);
        }
    }

    if (sig == SIGUSR2) {
        int valeur = info->si_value.sival_int;
        int type_evenement = (valeur >> 16) & 0xFFFF;
        int cle_recue = valeur & 0xFFFF;

        if (cle_recue == client->film_id) {
            if (type_evenement == 1) {
                snprintf(log_msg, sizeof(log_msg), "Début du film reçu avec la bonne clé pour le client %d\n", client->id);
                log_action(log_msg);
                strcpy(client->status, "regarde un film");
            } else if (type_evenement == 2) {
                snprintf(log_msg, sizeof(log_msg), "Fin du film reçue avec la bonne clé pour le client %d\n", client->id);
                log_action(log_msg);
                strcpy(client->status, "libre");
            }
        } else {
            snprintf(log_msg, sizeof(log_msg), "Signal reçu avec une clé incorrecte (%d) pour le client %d\n", cle_recue, client->id);
            log_action(log_msg);
        }
    }
}

// Réinitialisation du client après la fin du film
void reset_client(Client *client) {
    client->film_id = rand() % 3;
    client->age = rand() % 100;
    strcpy(client->status, "libre");
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
