#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_THREADS 20

typedef struct {
    int start;
    int end;
    int thread_id;
    int socket;
    char output_file[256];
} Segment;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int next_thread_to_write = 0;

void *receive_file_segment(void *args) {
    Segment *segment = (Segment *)args;

    FILE *file = fopen(segment->output_file, "r+b");
    if (!file) {
        perror("Error opening output file");
        pthread_exit(NULL);
    }

    fseek(file, segment->start, SEEK_SET);
    //fseek(file, segment->end, SEEK_SET);
    char buffer[BUFFER_SIZE];
    int bytes_to_receive = segment->end - segment->start;

    printf("Thread %d: Receiving segment from %d to %d\n", segment->thread_id, segment->start, segment->end);

    while (bytes_to_receive > 0) {
        int chunk_size = (bytes_to_receive < BUFFER_SIZE) ? bytes_to_receive : BUFFER_SIZE;
        int received = recv(segment->socket, buffer, chunk_size, 0);
        if (received < 0) {
            perror("Error receiving data");
            break;
        } else if (received == 0) {
            printf("Connection closed by the server\n");
            break;
        }

        pthread_mutex_lock(&mutex);
        while (next_thread_to_write != segment->thread_id) {
            pthread_cond_wait(&cond, &mutex);
        }

        fwrite(buffer, 1, received, file);
        bytes_to_receive -= received;

        if (bytes_to_receive <= 0) {
            next_thread_to_write++;
            pthread_cond_broadcast(&cond);
        }

        pthread_mutex_unlock(&mutex);
    }

    fclose(file);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <file_name> <num_threads> <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *file_name = argv[1];
    int num_threads = atoi(argv[2]);
    char *output_file = argv[3];

    if (num_threads <= 0 || num_threads > MAX_THREADS) {
        fprintf(stderr, "Error: Invalid number of threads: %d\n", num_threads);
        exit(EXIT_FAILURE);
    }

    int client_socket;
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Send file name
    printf("Sending file name: %s\n", file_name);
    if (send(client_socket, file_name, strlen(file_name) + 1, 0) <= 0) {
        perror("Error sending file name");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Send number of threads
    printf("Sending number of threads: %d\n", num_threads);
    if (send(client_socket, &num_threads, sizeof(num_threads), 0) <= 0) {
        perror("Error sending number of threads");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Receive file size
    int file_size;
    if (recv(client_socket, &file_size, sizeof(file_size), 0) <= 0) {
        perror("Error receiving file size");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    printf("File size received: %d bytes\n", file_size);

    FILE *file = fopen(output_file, "wb");
    if (!file) {
        perror("Error creating output file");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    fclose(file);

    int segment_size = file_size / num_threads;
    Segment segments[MAX_THREADS];
    pthread_t threads[MAX_THREADS];

    for (int i = 0; i < num_threads; i++) {
        segments[i].start = i * segment_size;
        segments[i].end = (i == num_threads - 1) ? file_size : (i + 1) * segment_size;
        segments[i].thread_id = i;
        segments[i].socket = client_socket;
        strcpy(segments[i].output_file, output_file);

        pthread_create(&threads[i], NULL, receive_file_segment, &segments[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    close(client_socket);
    printf("File transfer completed.\n");
    return 0;
}
