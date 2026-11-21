#include "connection_queue.h"

#include <stdio.h>
#include <string.h>

<<<<<<< HEAD
int connection_queue_init(connection_queue_t *queue) {
    // TODO Not yet implemented
    return 0;
}

int connection_queue_enqueue(connection_queue_t *queue, int connection_fd) {
    // TODO Not yet implemented
    return 0;
}

int connection_queue_dequeue(connection_queue_t *queue) {
    // TODO Not yet implemented
    return 0;
}

int connection_queue_shutdown(connection_queue_t *queue) {
    // TODO Not yet implemented
    return 0;
}

int connection_queue_free(connection_queue_t *queue) {
    // TODO Not yet implemented
    return 0;
=======
int connection_queue_init(connection_queue_t *queue)
{
    queue->length = 0;    // No connectiosn waiting
    queue->read_idx = 0;  // start dequeue
    queue->write_idx = 0; // start enqueue
    queue->shutdown = 0;  // active queue

    // initializes the mutex and condition variables
    if (pthread_mutex_init(&queue->mutex, NULL) != 0)
    {
        return -1; // creates the lock
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0)
    {
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }
    if (pthread_cond_init(&queue->not_full, NULL) != 0)
    {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }
    return 0;
}
int connection_queue_enqueue(connection_queue_t *queue, int connection_fd)
{
    // locks the queue before modifying it
    pthread_mutex_lock(&queue->mutex);
    // waits until there is space in the queue
    while (queue->length == CAPACITY && !queue->shutdown)
    {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    // stops if the queue has been shut down
    if (queue->shutdown)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    // adds the connection and update the write index
    queue->client_fds[queue->write_idx] = connection_fd;
    queue->write_idx = (queue->write_idx + 1) % CAPACITY;
    queue->length++;
    // Wake a thread waiting for a connection
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

int connection_queue_dequeue(connection_queue_t *queue)
{
    // locks the queue before accessing it
    pthread_mutex_lock(&queue->mutex);
    // waits until a connection is available
    while (queue->length == 0 && !queue->shutdown)
    {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    // stops if the queue has been shut down
    if (queue->shutdown)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    // removes the connection and update the read index
    int connection_fd = queue->client_fds[queue->read_idx];
    queue->read_idx = (queue->read_idx + 1) % CAPACITY;
    queue->length--;
    // Notify threads that space is available
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    return connection_fd;
}

int connection_queue_shutdown(connection_queue_t *queue)
{
    // locks the queue before changing its state
    pthread_mutex_lock(&queue->mutex);

    // marks the queue as shut down
    queue->shutdown = 1;

    // Wake up all waiting threads
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

int connection_queue_free(connection_queue_t *queue)
{
    // cleans up the mutex and condition variables
    int result = 0;
    if (pthread_cond_destroy(&queue->not_empty) != 0)
    {
        result = -1;
    }
    if (pthread_cond_destroy(&queue->not_full) != 0)
    {
        result = -1;
    }
    if (pthread_mutex_destroy(&queue->mutex) != 0)
    {
        result = -1;
    }
    return result;
>>>>>>> 8f2d122db87202f1b8391a830119faddc479b371
}
