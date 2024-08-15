#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define THREAD_POOL_SIZE 10

void *handle_request(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[1025] = {0};
    char response[1024] = {0};

    // read request
    read(client_socket, buffer, 1024);
    printf("Received request:\n%s\n", buffer);

    // prepare response
    snprintf(response, 1024, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n"
                             "<html><body><h1>Registration Page</h1>"
                             "<form action=\"register\" method=\"post\">"
                             "<label for=\"username\">Username:</label>"
                             "<input type=\"text\" id=\"username\" name=\"username\"><br><br>"
                             "<label for=\"password\">Password:</label>"
                             "<input type=\"password\" id=\"password\" name=\"password\"><br><br>"
                             "<input type=\"submit\" value=\"Submit\">"
                             "</form></body></html>");

    // send response
    write(client_socket, response, strlen(response));

    // close connection
    close(client_socket);

    // write data to file
    FILE *fp;
    fp = fopen("data.txt", "a");
    if (fp == NULL) {
        perror("File could not be opened");
        exit(EXIT_FAILURE);
    }

    char *data = strstr(buffer, "username");
    if (data != NULL) {
        data = strchr(data, '=') + 1;
        char *end = strchr(data, '&');
        if (end == NULL) {
            end = strchr(data, ' ');
        }
        *end = '\0';
        fprintf(fp, "%s,", data);
    }

    data = strstr(buffer, "password");
    if (data != NULL) {
        data = strchr(data, '=') + 1;
        char *end = strchr(data, '&');
        if (end == NULL) {
            end = strchr(data, ' ');
        }
        *end = '\0';
        fprintf(fp, "%s,", data);
    }

    fclose(fp);

    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    int opt = 1;
    int addrlen = sizeof(server_address);
    int i;

    // create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // bind socket to port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // listen for connections
    if (listen(server_socket, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", PORT);

        // create thread pool
    pthread_t threads[THREAD_POOL_SIZE];
    int thread_args[THREAD_POOL_SIZE];
    for (i = 0; i < THREAD_POOL_SIZE; i++) {
        thread_args[i] = 0;
    }

    // accept connections and assign to threads
    while (1) {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        printf("Client connected\n");
        for (i = 0; i < THREAD_POOL_SIZE; i++) {
            if (!thread_args[i]) {
                thread_args[i] = client_socket;
                pthread_create(&threads[i], NULL, handle_request, &thread_args[i]);
                break;
            }
        }
        if (i == THREAD_POOL_SIZE) {
            printf("No available threads\n");
            close(client_socket);
        }
    }
    return 0;
}