#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define MAX_PROCS 4
#define PORT_BASE 5000
#define MAX_EVENTS 5
#define MAX_MSGS 4

int ports[MAX_PROCS] = {5000, 5001, 5002, 5003};
int clock_time = 0;
int my_id;

pthread_mutex_t clock_lock = PTHREAD_MUTEX_INITIALIZER;

void *receiver_thread(void *arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[32];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(ports[my_id]);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        memset(buffer, 0, sizeof(buffer));
        read(new_socket, buffer, sizeof(buffer));
        int received_time = atoi(buffer);

        pthread_mutex_lock(&clock_lock);
        if (received_time >= clock_time)
            clock_time = received_time + 1;
        else
            clock_time++;
        printf("P%d REÇOIT horloge=%d → horloge MAJ=%d\n", my_id, received_time, clock_time);
        pthread_mutex_unlock(&clock_lock);
        close(new_socket);
    }
    return NULL;
}

void send_message(int to_id) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[32];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ports[to_id]);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // attendre que l'autre processus soit prêt
    while (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        usleep(100000); 
    }

    pthread_mutex_lock(&clock_lock);
    clock_time++;
    sprintf(buffer, "%d", clock_time);
    send(sockfd, buffer, strlen(buffer), 0);
    printf("P%d ENVOIE horloge=%d à P%d\n", my_id, clock_time, to_id);
    pthread_mutex_unlock(&clock_lock);

    close(sockfd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <proc_id 0-3>\n", argv[0]);
        exit(1);
    }

    my_id = atoi(argv[1]);
    pthread_t recv_thread;

    pthread_create(&recv_thread, NULL, receiver_thread, NULL);

    sleep(1); // laisser le temps au serveur d'écouter

    for (int i = 0; i < MAX_EVENTS; i++) {
        pthread_mutex_lock(&clock_lock);
        clock_time++;
        printf("P%d ÉVÉNEMENT LOCAL %d → horloge=%d\n", my_id, i + 1, clock_time);
        pthread_mutex_unlock(&clock_lock);

        if (i < MAX_MSGS) {
            int to_id = (my_id + i + 1) % MAX_PROCS;
            send_message(to_id);
        }

        sleep(2); // attendre entre les événements
    }

    sleep(5); // laisser temps aux messages d'arriver
    printf("P%d FIN avec horloge finale = %d\n", my_id, clock_time);
    return 0;
}

