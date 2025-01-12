#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "producer-consumer.h"

int pcq_create(pc_queue_t *queue, size_t capacity)
{
    queue->pcq_buffer = malloc(capacity * sizeof(void *));
    if (queue->pcq_buffer == NULL)
    {
        return -1;
    }

    // set the values to 0 and capacity to what's given by the user 
    queue->pcq_capacity = capacity;
    queue->pcq_current_size = 0;
    queue->pcq_head = 0;
    queue->pcq_tail = 0;

    // initialize all mutexes
    pthread_mutex_init(&queue->pcq_current_size_lock, NULL);
    pthread_mutex_init(&queue->pcq_head_lock, NULL);
    pthread_mutex_init(&queue->pcq_tail_lock, NULL);
    pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL);
    pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL);

    return 0;
}

int pcq_destroy(pc_queue_t *queue)
{
    // free buffer
    free(queue->pcq_buffer);
    
    // destroy all mutexes
    pthread_mutex_destroy(&queue->pcq_current_size_lock);
    pthread_mutex_destroy(&queue->pcq_head_lock);
    pthread_mutex_destroy(&queue->pcq_tail_lock);
    pthread_mutex_destroy(&queue->pcq_popper_condvar_lock);
    pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock);

    return 0;
}

int pcq_enqueue(pc_queue_t *queue, void *elem)
{
    pthread_mutex_lock(&queue->pcq_current_size_lock);

    while (&queue->pcq_current_size == &queue->pcq_capacity)
    {
        pthread_cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_current_size_lock);
    }

    queue->pcq_current_size++;
    pthread_cond_signal(&queue->pcq_popper_condvar);
    pthread_mutex_unlock(&queue->pcq_current_size_lock);

    pthread_mutex_lock(&queue->pcq_head_lock);
    queue->pcq_buffer[queue->pcq_head] = elem;
    queue->pcq_head = (queue->pcq_head + 1) % queue->pcq_capacity;
    pthread_mutex_unlock(&queue->pcq_head_lock);

    return 0;
}

void *pcq_dequeue(pc_queue_t *queue)
{
    pthread_mutex_lock(&queue->pcq_current_size_lock);

    while (queue->pcq_current_size == 0)
    {
        pthread_cond_wait(&queue->pcq_popper_condvar, &queue->pcq_current_size_lock);
    }

    queue->pcq_current_size--;
    pthread_cond_signal(&queue->pcq_pusher_condvar);
    pthread_mutex_unlock(&queue->pcq_current_size_lock);

    pthread_mutex_lock(&queue->pcq_tail_lock);
    queue->pcq_tail = (queue->pcq_tail + 1) % queue->pcq_capacity;
    pthread_mutex_unlock(&queue->pcq_tail_lock);

    return 0;
}
