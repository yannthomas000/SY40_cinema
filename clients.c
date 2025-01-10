#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define NUM_CLIENTS 3
#define SHM_KEY 1234
#define SEM_KEY 5678

// Structure pour les messages échangés entre les processus
struct message {
    long message_type;
    int pid;
    int film_id;
    int age;
};

// Structure pour représenter un client
typedef struct {
    int id;
    int age;
    char status[50];
    int film_id;
} Client;

// Structure pour la mémoire partagée
typedef struct {
    Client clients[NUM_CLIENTS];
} SharedData;

volatile sig_atomic_t cleanup_done = 0;

// Prototypes des fonctions
Client create_client(int id, int age);
bool reserver_film(Client *client);
void gestionnaire_signal(int sig, siginfo_t *info, void *context);
void reset_client(Client *client);
void log_action(const char *message);
void client_process(Client client);
void delete_message_queue(int msgid);
void handle_sigint(int sig);

void init_shared_memory(SharedData **shared_data, int *shmid);
void attach_shared_memory(int shmid, SharedData **shared_data);
void detach_shared_memory(SharedData *shared_data);
void remove_shared_memory(int shmid);
void init_semaphore(int *semid);
void wait_semaphore(int semid);
void signal_semaphore(int semid);
void remove_semaphore(int semid);

int main(void) {
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    pid_t pids[NUM_CLIENTS];
    int i;

    // Initialisation de la mémoire partagée et des sémaphores
    SharedData *shared_data = NULL;
    int shmid, semid;
    init_shared_memory(&shared_data, &shmid);
    init_semaphore(&semid);

    // Enregistrement du gestionnaire de signal
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = gestionnaire_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    // Création des processus enfants pour chaque clients
    for (i = 0; i < NUM_CLIENTS; i++) {
        if ((pids[i] = fork()) < 0) {
            perror("fork");
            exit(1);
        } else if (pids[i] == 0) {
            // Code du processus enfant
            time_t t = time(NULL); // Obtenir le temps actuel
            srand(t + getpid()); // Utiliser le temps actuel et le PID comme graine pour rand()
            wait_semaphore(semid);
            Client client = create_client(getpid(), rand() % 100);
            shared_data->clients[i] = client;
            signal_semaphore(semid);
            client_process(client);
            exit(0);
        }
    }

    // Attente de la fin de tous les processus enfants
    for (i = 0; i < NUM_CLIENTS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    log_action("Tous les clients ont terminé leurs activités.\n");
    printf("Tous les clients ont terminé leurs activités.\n");

    // Suppression de la file de messages
    key_t key1 = 17;
    int msgid = msgget(key1, 0666);
    if (msgid >= 0) {
        delete_message_queue(msgid);
    }

    remove_shared_memory(shmid);
    remove_semaphore(semid);

    return 0;
}

// Fonction pour la création d'un nouveau client
Client create_client(int id, int age) {
    Client client;
    client.id = id;
    client.age = age;
    strcpy(client.status, "libre");
    client.film_id = -1;
    printf("Client %d créé avec priorité %d et âge %d\n", id, age);
    return client;
}

// Fonction pour le processus de chaque client
void client_process(Client client) {
    char log_msg[100];
    while (1) {
        // Attendre entre 5 et 10 secondes
        printf("Attente du client %d\n", client.id);
        sleep(5 + rand() % 6);

        // Réservation d'un film à travers une file de messages
        if (reserver_film(&client)) {
            snprintf(log_msg, sizeof(log_msg), "Client %d a réservé un film\n", client.id);
            printf("Client %d a réservé un film\n", client.id);
            log_action(log_msg);
        }

        // Attendre la confirmation de réservation
        pause();

        if (strcmp(client.status, "attend") == 0) {
            // Attendre le début du film
            printf("Attente du début du film pour le client %d\n", client.id);
            pause();

            if (strcmp(client.status, "regarde un film") == 0) {
                // Attendre la fin du film
                printf("Attente de la fin du film pour le client %d\n", client.id);
                pause();
            }
        }

        // Réinitialisation du client après la fin du film ou échec de réservation
        reset_client(&client);
        snprintf(log_msg, sizeof(log_msg), "Client %d réinitialisé\n", client.id);
        log_action(log_msg);
    }
}

// Fonction pour réserver un film en envoyant un message dans une file de messages
bool reserver_film(Client *client) {
    int msgid, flag;
    key_t key1 = 17;
    flag = IPC_CREAT | 0666;
    struct message msg;

    // Création de la file de messages
    msgid = msgget(key1, flag);
    if (msgid < 0) {
        // En cas d'erreur, retourner false
        perror("Erreur lors de la création de la file de messages");
        return false;
    }

    // Préparation du message
    msg.message_type = 2;
    msg.pid = getpid();
    msg.film_id = rand() % 3;
    msg.age = client->age;

    // Envoi du message dans la file
    if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), IPC_NOWAIT) < 0) {
        perror("Erreur lors de l'envoi du message");
        return false;
    }
    printf("Message envoyé par le client %d\n", client->id);
    return true;
}

// Gestionnaire de signal pour les signaux SIGUSR1 et SIGUSR2
void gestionnaire_signal(int sig, siginfo_t *info, void *context) {
    int client_pid = info->si_pid;
    printf("Gestionnaire de signal: reçu par le client %d\n", client_pid);
    SharedData *shared_data = NULL;
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666); // Get the shmid

    if (shmid < 0) {
        perror("Error getting shared memory ID");
        return;
    }

    // Attach the shared memory
    attach_shared_memory(shmid, &shared_data);

    // Find the client corresponding to the PID
    Client *client = NULL;
    for (int i = 0; i < NUM_CLIENTS; i++) {
        if (shared_data->clients[i].id == client_pid) {
            client = &shared_data->clients[i];
            break;
        }
    }

    // If the client is not found, display an error and return
    if (client == NULL) {
        perror("Client not found");
        detach_shared_memory(shared_data);
        return;
    }

    char log_msg[100];
    if (sig == SIGUSR1) { // Confirmation de réservation reçue
        int film_id = info->si_value.sival_int;
        if (film_id == 999) {
            // Si le client est trop jeune pour le film, réinitialiser le client
            snprintf(log_msg, sizeof(log_msg), "Le client %d est trop jeune pour le film\n", client->id);
            printf("Le client %d est trop jeune pour le film\n", client->id);
            log_action(log_msg);
            reset_client(client);
        } else if(film_id == 888){
            // Si la salle est pleine, réinitialiser le client
            snprintf(log_msg, sizeof(log_msg), "La salle est pleine pour le client %d\n", client->id);
            printf("La salle est pleine pour le client %d\n", client->id);
            log_action(log_msg);
            reset_client(client);
        }else{
            // Si la réservation est confirmée, mettre à jour le statut du client
            client->film_id = film_id;
            strcpy(client->status, "attend");
            snprintf(log_msg, sizeof(log_msg), "Client %d attend pour le film %d\n", client->id, film_id);
            printf("Client %d attend pour le film %d\n", client->id, film_id);
            log_action(log_msg);
        }
    }

    if (sig == SIGUSR2) { // Signal de début ou fin de film reçu
        // Extraire la clé et le type d'événement du signal
        int valeur = info->si_value.sival_int;
        int type_evenement = (valeur >> 16) & 0xFFFF;
        int cle_recue = valeur & 0xFFFF;

        if (cle_recue == client->film_id) { // Vérifier la clé
            if (type_evenement == 1) {
                //Si le signal est de début de film, mettre à jour le statut du client
                snprintf(log_msg, sizeof(log_msg), "Début du film reçu avec la bonne clé pour le client %d\n", client->id);
                printf("Début du film reçu avec la bonne clé pour le client %d\n", client->id);
                log_action(log_msg);
                strcpy(client->status, "regarde un film");
            } else if (type_evenement == 2) {
                //Si le signal est de fin de film, mettre à jour le statut du client
                snprintf(log_msg, sizeof(log_msg), "Fin du film reçue avec la bonne clé pour le client %d\n", client->id);
                printf("Fin du film reçue avec la bonne clé pour le client %d\n", client->id);
                log_action(log_msg);
                strcpy(client->status, "libre");
            }
        } else {
            // Si la clé est incorrecte, afficher un message d'erreur
            snprintf(log_msg, sizeof(log_msg), "Signal reçu avec une clé incorrecte (%d) pour le client %d\n", cle_recue, client->id);
            printf("Signal reçu avec une clé incorrecte (%d) pour le client %d\n", cle_recue, client->id);
            log_action(log_msg);
        }
    }
    // Détacher la mémoire partagée
    detach_shared_memory(shared_data);
}

// Fonction pour réinitialiser un client
void reset_client(Client *client) {
    client->film_id = rand() % 3; // Choisir un nouveau film aléatoire
    client->age = rand() % 100; // Choisir un nouvel âge aléatoire entre 0 et 99
    strcpy(client->status, "libre"); // Mettre à jour le statut

    // Vérifier si l'ID de film est valide
    if (client->film_id < 0 || client->film_id >= 3) {
        log_action("Erreur : ID de film invalide après réinitialisation.\n");
        reset_client(client);
    }
    printf("Client %d réinitialisé\n", client->id);
}

// Fonction pour supprimer une file de messages
void delete_message_queue(int msgid) {
    if (msgctl(msgid, IPC_RMID, NULL) < 0) {
        perror("Erreur lors de la suppression de la file de messages");
    } else {
        log_action("File de messages supprimée avec succès.\n");
    }
}

// Fonction pour enregistrer une action dans un fichier de log
void log_action(const char *message) {
    // Ouvrir le fichier de log en mode append
    FILE *log_file = fopen("log.txt", "a");
    if (log_file != NULL) {
        // Verrouiller le fichier pour éviter les écritures simultanées
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

// Function to handle SIGINT
void handle_sigint(int sig) {
    if (cleanup_done) {
        return;
    }
    cleanup_done = 1;

    printf("Signal SIGINT reçu, arrêt du programme...\n");

    // Suppression de la mémoire partagée
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if (shmid >= 0) {
        remove_shared_memory(shmid);
    }

    // Suppression du sémaphore
    int semid = semget(SEM_KEY, 1, 0666);
    if (semid >= 0) {
        remove_semaphore(semid);
    }

    // Suppression de la file de messages
    key_t key1 = 17;
    int msgid = msgget(key1, 0666);
    if (msgid >= 0) {
        delete_message_queue(msgid);
    }

    exit(0);
}

// Function to initialize shared memory
void init_shared_memory(SharedData **shared_data, int *shmid) {
    printf("Initializing shared memory\n");
    *shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (*shmid < 0) {
        perror("Error creating shared memory");
        exit(1);
    }
    *shared_data = (SharedData *)shmat(*shmid, NULL, 0);
    if (*shared_data == (SharedData *)-1) {
        perror("Error attaching shared memory");
        exit(1);
    }
}

// Function to attach shared memory
void attach_shared_memory(int shmid, SharedData **shared_data) {
    *shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (*shared_data == (SharedData *)-1) {
        perror("Error attaching shared memory");
        exit(1);
    }
}

// Fonction pour détacher la mémoire partagée
void detach_shared_memory(SharedData *shared_data) {
    if (shmdt(shared_data) < 0) {
        perror("Erreur lors du détachement de la mémoire partagée");
    }
}

// Fonction pour supprimer la mémoire partagée
void remove_shared_memory(int shmid) {
    printf("Suppression de la mémoire partagée\n");
    if (shmctl(shmid, IPC_RMID, NULL) < 0) {
        perror("Erreur lors de la suppression de la mémoire partagée");
    }
}

// Fonction pour initialiser le sémaphore
void init_semaphore(int *semid) {
    printf("Initialisation du sémaphore\n");
    *semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (*semid < 0) {
        perror("Erreur lors de la création du sémaphore");
        exit(1);
    }
    if (semctl(*semid, 0, SETVAL, 1) < 0) {
        perror("Erreur lors de l'initialisation du sémaphore");
        exit(1);
    }
}

// Fonction pour attendre le sémaphore
void wait_semaphore(int semid) {
    struct sembuf sb = {0, -1, 0};
    if (semop(semid, &sb, 1) < 0) {
        perror("Erreur lors de l'attente du sémaphore");
    }
}

// Fonction pour signaler le sémaphore
void signal_semaphore(int semid) {
    struct sembuf sb = {0, 1, 0};
    if (semop(semid, &sb, 1) < 0) {
        perror("Erreur lors du signal du sémaphore");
    }
}

// Fonction pour supprimer le sémaphore
void remove_semaphore(int semid) {
    printf("Suppression du sémaphore\n");
    if (semctl(semid, 0, IPC_RMID) < 0) {
        perror("Erreur lors de la suppression du sémaphore");
    }
}
