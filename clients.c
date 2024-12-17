#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

#define NUM_CLIENTS 5
#define NUM_SALLES 5

struct message {
    long message_type;
    int pid;
    int film_id;
};

typedef struct {
    int id;                 // Identifiant unique du client
    int priority;           // Numéro de priorité dans la file
    int age;                // Âge du client
    char status[50];        // Statut actuel du client ("regarde un film", "attend", "libre")
    int film_id;            // Identifiant du film regardé
}Client;

Client create_client(int id, int priority, int age);
bool reserver_film(Client *client);
void attendre_projection(Client *client);
void regarder_film(Client *client);
void reset_client(Client *client);

int main(void) {
    pid_t pids[NUM_CLIENTS];
    int i;

    for (i = 0; i < NUM_CLIENTS; i++) {
        if ((pids[i] = fork()) < 0) {
            perror("fork");
            exit(1);
        } else if (pids[i] == 0) {
            // Code du processus enfant
            Client client = create_client(i + 1, rand() % 10, rand() % 100);
            printf("Client %d créé avec priorité %d et âge %d\n", client.id, client.priority, client.age);

            // Réservation d'un film
            if (reserver_film(&client)) {
                printf("Client %d a réservé un film\n", client.id);
            }
            
            // Simulation de regarder un film
            regarder_film(&client);

            // Réinitialisation du client après la fin du film
            reset_client(&client);

            exit(0);
        }
    }

    // Attente de la fin de tous les processus enfants
    for (i = 0; i < NUM_CLIENTS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    printf("Tous les clients ont terminé leurs activités.\n");

    return 0;
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
    // Choisis un film aléatoire
    msg.film_id = rand() % 3;

    msgsnd(msgid, &msg, sizeof(msg), IPC_NOWAIT);

    return true;
}

void gestionnaire_signal(int sig, siginfo_t *info, void *context, Client *client) {
    if (sig == SIGUSR1) {
        int film_id = info->si_value.sival_int;
        client->film_id = film_id; // Enregistrement de l'ID du film
        strcpy(client->status, "attend"); // Mise à jour du statut
    }

    if (sig == SIGUSR2) {
        int valeur = info->si_value.sival_int;

        // Décodage : Extraire clé et type d'événement
        int type_evenement = (valeur >> 16) & 0xFFFF; // Bits supérieurs
        int cle_recue = valeur & 0xFFFF;             // Bits inférieurs

        // Vérification de la clé
        if (cle_recue == client->film_id) {
            if (type_evenement == 1) {
                printf("Début du film reçu avec la bonne clé !\n");
                strcpy(client->status, "regarde un film");
            } else if (type_evenement == 0) {
                printf("Fin du film reçue avec la bonne clé !\n");
                strcpy(client->status, "libre");
            }
        } else {
            printf("Signal reçu avec une clé incorrecte (%d) !\n", cle_recue);
        }
    }
}

// Réinitialisation du client après la fin du film
void reset_client(Client *client) {
    strcpy(client->status, "libre");
    client->film_id = -1;
}
