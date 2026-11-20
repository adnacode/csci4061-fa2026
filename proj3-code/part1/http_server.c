#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection_queue.h"
#include "http.h"

#define N_THREADS 5

int keep_going = 1;
const char *serve_dir;

void handle_sigint(int signo)
{
    keep_going = 0;
}

void *worker_func(void *arg)
{
    connection_queue_t *queue = (connection_queue_t *)arg;
    while (1)
    {
        // retreieves a client connection from the queue
        int client_fd = connection_queue_dequeue(queue);
        if (client_fd == -1)
        {
            break;
        }
        char resource_name[4096];
        char resource_path[8192];

        // goes to read the client's HTTP request
        if (read_http_request(client_fd, resource_name) == 0)
        {
            // Build the local file path
            int length = snprintf(resource_path, sizeof(resource_path),
                                  "%s%s", serve_dir, resource_name);

            if (length >= 0 && (size_t)length < sizeof(resource_path))
            {
                write_http_response(client_fd, resource_path);
            }
        }
        // close the connection
        close(client_fd);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    // Uncomment the lines below to use these definitions
    serve_dir = argv[1];
    const char *port = argv[2];

    // creates address information for the TCP server
    struct addrinfo hints, *server_info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // grabs address information for the specified port
    int result = getaddrinfo(NULL, port, &hints, &server_info);
    if (result != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        return 1;
    }
    // crafts the server socket
    int server_fd = socket(server_info->ai_family,
                           server_info->ai_socktype,
                           server_info->ai_protocol);
    if (server_fd == -1)
    {
        perror("socket");
        freeaddrinfo(server_info);
        return 1;
    }

    // binds the socket to the port
    if (bind(server_fd, server_info->ai_addr,
             server_info->ai_addrlen) == -1)
    {
        perror("bind");
        close(server_fd);
        freeaddrinfo(server_info);
        return 1;
    }

    freeaddrinfo(server_info);
    // checks and starts listening for incoming connections
    if (listen(server_fd, 10) == -1)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    // clean shutdown
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        close(server_fd);
        return 1;
    }

    // sets up the connection queue for incoming connections
    connection_queue_t queue;

    if (connection_queue_init(&queue) == -1)
    {
        close(server_fd);
        return 1;
    }

    // worker threads to handle client requests
    pthread_t workers[N_THREADS];

    for (int i = 0; i < N_THREADS; i++)
    {
        if (pthread_create(&workers[i], NULL, worker_func, &queue) != 0)
        {
            perror("pthread_create");
            connection_queue_shutdown(&queue);

            for (int j = 0; j < i; j++)
            {
                pthread_join(workers[j], NULL);
            }

            connection_queue_free(&queue);
            close(server_fd);
            return 1;
        }
    }

    // lets in incoming client connections
    while (keep_going)
    {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1)
        {
            if (errno == EINTR && !keep_going)
            {
                break;
            }

            perror("accept");
            continue;
        }
        // add the connection to the queue
        if (connection_queue_enqueue(&queue, client_fd) == -1)
        {
            close(client_fd);
            break;
        }
    }
    // shuts down the queue and wake awaiting workers
    connection_queue_shutdown(&queue);
    // wait for all worker threads to finish
    for (int i = 0; i < N_THREADS; i++)
    {
        pthread_join(workers[i], NULL);
    }
    // clean up
    connection_queue_free(&queue);
    close(server_fd);

    return 0;
}
