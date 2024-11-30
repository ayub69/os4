#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_THREADS 20

typedef struct {
    int start;
    int end;
    int thread_id;
    int client_socket;
    char file_name[256];
} Segment;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int next_thread_to_send = 0; // Keeps track of the next thread allowed to send data

void *send_file_segment(void *args) {
    Segment *segment = (Segment *)args;

    FILE *file = fopen(segment->file_name, "rb");
    if (!file) {
        perror("Error opening file");
        pthread_exit(NULL);
    }

    fseek(file, segment->start, SEEK_SET);
    char buffer[BUFFER_SIZE];
    int bytes_to_send = segment->end - segment->start;

    // Wait until it's this thread's turn to send
    pthread_mutex_lock(&mutex);
    while (next_thread_to_send != segment->thread_id) {
        pthread_cond_wait(&cond, &mutex);
    }

    printf("Thread %d: Sending segment from %d to %d\n", segment->thread_id, segment->start, segment->end);

    while (bytes_to_send > 0) {
        int chunk_size = (bytes_to_send < BUFFER_SIZE) ? bytes_to_send : BUFFER_SIZE;
        int read_bytes = fread(buffer, 1, chunk_size, file);
        if (read_bytes <= 0) break;

        if (send(segment->client_socket, buffer, read_bytes, 0) <= 0) {
            perror("Error sending data");
            break;
        }

        bytes_to_send -= read_bytes;
    }

    fclose(file);

    // Signal the next thread to send
    next_thread_to_send++;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
    if (client_socket < 0) {
        perror("Connection failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    char file_name[256];
    int num_threads;

    // Receive file name and number of threads
    recv(client_socket, file_name, sizeof(file_name), 0);
    recv(client_socket, &num_threads, sizeof(num_threads), 0);

    FILE *file = fopen(file_name, "rb");
    if (!file) {
        perror("Error opening file");
        close(client_socket);
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fclose(file);

    int segment_size = file_size / num_threads;
    Segment segments[MAX_THREADS];
    pthread_t threads[MAX_THREADS];

    for (int i = 0; i < num_threads; i++) {
        segments[i].start = i * segment_size;
        segments[i].end = (i == num_threads - 1) ? file_size : (i + 1) * segment_size;
        segments[i].thread_id = i;
        segments[i].client_socket = client_socket;
        strcpy(segments[i].file_name, file_name);

        pthread_create(&threads[i], NULL, send_file_segment, &segments[i]);
        pthread_join(threads[i], NULL);
    }

    // for (int i = 0; i < num_threads; i++) {
    //     pthread_join(threads[i], NULL);
    // }

    close(client_socket);
    close(server_socket);
    printf("Server shut down.\n");
    return 0;
}
