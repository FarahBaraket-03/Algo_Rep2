#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define NUM_PROCESSES 4
#define MAX_EVENTS 9
#define BASE_PORT 5000

int vector_clock[NUM_PROCESSES];
int pid;

void print_vector() {
    printf("P%d - [", pid);
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("%d", vector_clock[i]);
        if (i < NUM_PROCESSES - 1) printf(", ");
    }
    printf("]\n");
}

void update_vector(int *msg) {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (msg[i] > vector_clock[i]) {
            vector_clock[i] = msg[i];
        }
    }
}

void local_event() {
    vector_clock[pid]++;
    printf("P%d - Événement local\n", pid);
    print_vector();
}

void send_message(int target_pid) {
    vector_clock[pid]++;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in target_addr;
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(BASE_PORT + target_pid);
    target_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while (connect(sockfd, (struct sockaddr*)&target_addr, sizeof(target_addr)) < 0) {
        usleep(500000); // attendre que le serveur soit prêt
    }

    send(sockfd, vector_clock, sizeof(vector_clock), 0);
    close(sockfd);
    printf("P%d - Message envoyé à P%d\n", pid, target_pid);
    print_vector();
}

void receive_message(int server_sock) {
    int new_sock = accept(server_sock, NULL, NULL);
    int msg[NUM_PROCESSES];
    recv(new_sock, msg, sizeof(msg), 0);
    close(new_sock);
    
    vector_clock[pid]++;
    update_vector(msg);
    printf("P%d - Message reçu\n", pid);
    print_vector();
}

int setup_server() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(BASE_PORT + pid);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sockfd, 5);
    return sockfd;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <pid>\n", argv[0]);
        exit(1);
    }

    pid = atoi(argv[1]);
    for (int i = 0; i < NUM_PROCESSES; i++) vector_clock[i] = 0;

    int server_sock = setup_server();
    sleep(1); // Attendre que tous les serveurs soient prêts

    if (pid == 0) {
    	local_event();
    	send_message(1);
    	local_event();
    	send_message(2);
    	local_event();
    } else if (pid == 1) {
    	receive_message(server_sock);
    	local_event();
    	send_message(3);
    	local_event();
    	local_event();
    } else if (pid == 2) {
    	receive_message(server_sock);
    	local_event();
    	send_message(3);
    	local_event();
    	local_event();
    } else if (pid == 3) {
    	receive_message(server_sock);
    	receive_message(server_sock);
    	local_event();
    	local_event();
    	local_event();
}


    close(server_sock);
    return 0;
}
