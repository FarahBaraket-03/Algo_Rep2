#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define N 4
#define PORT_BASE 5000
#define MAX_EVENTS 9

int id; // ID du processus (0 à 3)
int mat_clock[N][N];

void print_matrix() {
    printf("Process %d Matrix Clock:\n", id);
    for (int i = 0; i < N; i++) {
        printf("[ ");
        for (int j = 0; j < N; j++) {
            printf("%d ", mat_clock[i][j]);
        }
        printf("]\n");
    }
    printf("\n");
}

void update_on_receive(int recv_matrix[N][N], int sender_id) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if (mat_clock[i][j] < recv_matrix[i][j])
                mat_clock[i][j] = recv_matrix[i][j];

    mat_clock[id][id]++;
}

void send_matrix(int dest_id) {
    int sock;
    struct sockaddr_in server_addr;
    int port = PORT_BASE + dest_id;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket failed");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        usleep(100000); // retry
    }

    mat_clock[id][id]++;
    write(sock, mat_clock, sizeof(mat_clock));
    close(sock);

    printf("Process %d sent message to %d\n", id, dest_id);
    print_matrix();
}

void *server_thread(void *arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int port = PORT_BASE + id;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Server socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) continue;

        int recv_matrix[N][N];
        read(new_socket, recv_matrix, sizeof(recv_matrix));
        update_on_receive(recv_matrix, -1);

        printf("Process %d received message\n", id);
        print_matrix();
        close(new_socket);
    }
    return NULL;
}

void local_event() {
    mat_clock[id][id]++;
    printf("Process %d local event\n", id);
    print_matrix();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s [process_id 0-3]\n", argv[0]);
        return 1;
    }

    id = atoi(argv[1]);
    memset(mat_clock, 0, sizeof(mat_clock));

    pthread_t server;
    pthread_create(&server, NULL, server_thread, NULL);
    usleep(500000); // Wait for all servers

    // Example scenario: Each process has a fixed pattern
    sleep(1); local_event();
    sleep(1); local_event();

    if (id == 0) {
        sleep(1); send_matrix(1);
        sleep(1); local_event();
        sleep(1); send_matrix(2);
        sleep(1); local_event();
    } else if (id == 1) {
        sleep(2); local_event();
        sleep(2); send_matrix(3);
        sleep(2); local_event();
    } else if (id == 2) {
        sleep(3); local_event();
        sleep(2); send_matrix(3);
        sleep(2); local_event();
    } else if (id == 3) {
        sleep(4); local_event();
        sleep(2); local_event();
    }

    sleep(10); // Wait for all messages
    return 0;
}

