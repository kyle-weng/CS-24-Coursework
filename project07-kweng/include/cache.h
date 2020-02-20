#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "buffer.h"

/* Structs */
typedef struct node_t node_t;
typedef struct queue_t queue_t;
typedef struct hash_table_t hash_table_t;
typedef struct cache_t cache_t;

int hash(char *s);

/* Initialization methods */
node_t *init_node(char *key, buffer_t *value);
queue_t *init_queue();
hash_table_t *init_hash_table();
cache_t *init_cache();

/*
 * Queue methods -- some of these have the cache as a parameter, but that's
 * just to ensure that client_thread.c only deals with the cache itself. You
 * can obviously call the queue given the cache easily enough.
 */
bool is_empty(queue_t *queue);
node_t *dequeue(queue_t *queue, pthread_rwlock_t lock);
void enqueue(queue_t *queue, node_t *node, pthread_rwlock_t lock);
void queue_update(cache_t *cache, node_t *node);
buffer_t *peek(cache_t *cache);

/* Cache methods*/
node_t *cache_insert(cache_t *cache, char *key, buffer_t *value);
buffer_t *cache_remove(cache_t *cache);
node_t *contains(cache_t *cache, char *key, pthread_rwlock_t lock);

/* Getter methods */
pthread_rwlock_t get_lock(cache_t *cache);
queue_t *get_queue(cache_t *cache);

/* Free methods */
void free_hash_table(hash_table_t *hash_table);
void free_cache(cache_t *cache);

#endif // CACHE_H