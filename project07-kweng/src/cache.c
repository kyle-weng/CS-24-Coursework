#include "buffer.h"
#include "cache.h"
#include <pthread.h>

const size_t BINS = 1024;
const size_t MAX_CACHE_SIZE = 1024 * 1024;
const size_t MAX_OBJECT_SIZE = 1024 * 100;

struct node_t {
	// for use in the queue
	node_t *prev;
	node_t *next;

	// for use in the hash table
	node_t *hash_table_prev;
	node_t *hash_table_next;

	// for storing the (key, value) pair
	char *key;
	buffer_t *value;
};

struct queue_t {
	size_t size;
	node_t *front;
	node_t *back;
};

struct hash_table_t {
	size_t capacity;
	node_t **arr;
};

struct cache_t {
	size_t size;
	pthread_rwlock_t lock;
	queue_t *queue;
	hash_table_t *hash_table;
};

/* Hash a string. */
int hash(char *s) {
	uint8_t hash = (uint8_t)5381;
	int c;
	while ((c = *s++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return (int)hash % BINS;
}

/* Initialize a node with the given value. */
node_t *init_node(char *key, buffer_t *value) {
	node_t *node = (node_t *)malloc(sizeof(node_t));
	assert(node != NULL);
	node->key = key;
	node->value = value;
	node->prev = NULL;
	node->next = NULL;
	node->hash_table_prev = NULL;
	node->hash_table_next = NULL;
	return node;
}

/* Initialize a queue with the given capacity. */
queue_t *init_queue() {
	queue_t *queue = (queue_t *)malloc(sizeof(queue_t));
	assert(queue != NULL);
	queue->size = 0;
	queue->front = NULL;
	queue->back = NULL;
	return queue;
}

/* Initialize a hash table with a default capacity determined by BINS. */
hash_table_t *init_hash_table() {
	hash_table_t *hash_table = (hash_table_t *)malloc(sizeof(hash_table_t));
	assert(hash_table != NULL);
	hash_table->capacity = BINS;
	hash_table->arr = (node_t **)malloc(BINS * sizeof(node_t *));
	assert(hash_table->arr != NULL);
	for (size_t i = 0; i < hash_table->capacity; i++) {
		hash_table->arr[i] = NULL;
	}
	return hash_table;
}

/* Initialize the cache. */
cache_t *init_cache() {
	cache_t *cache = (cache_t *)malloc(sizeof(cache_t));
	assert(cache != NULL);
	queue_t *queue = init_queue();
	hash_table_t *hash_table = init_hash_table();
	pthread_rwlock_init(&cache->lock, NULL);
	cache->size = 0;
	cache->queue = queue;
	cache->hash_table = hash_table;
	return cache;
}

bool is_empty(queue_t *queue) {
	return queue->back == NULL;
}

/* Dequeue from the given queue and return the dequeued node. */
node_t *dequeue(queue_t *queue, pthread_rwlock_t lock) {
	if (is_empty(queue)) {
		return NULL;
	}

	pthread_rwlock_wrlock(&lock);

	// if there is only one item in the queue, set the front to NULL and return
	// the item
	if (queue->front == queue->back) {
		node_t *temp = queue->front;
		queue->front = NULL;
		pthread_rwlock_unlock(&lock);
		return temp;
	}
	
	node_t *temp = queue->back;
	temp = temp->prev;

	if (temp != NULL) {
		temp->next = NULL;
	}
	queue->size--;
	pthread_rwlock_unlock(&lock);
	return temp;
}

/* Move the given node to the front of the given queue. */
void queue_update(cache_t *cache, node_t *node) {
	pthread_rwlock_wrlock(&cache->lock);
	if (node == cache->queue->front) {
		// no updating necessary
		pthread_rwlock_unlock(&cache->lock);
		return;
	}
	node->prev->next = node->next;
	if (node->next != NULL) {
		node->next->prev = node->prev;
	}
	if (node == cache->queue->back) {
		cache->queue->back = node->prev;
		cache->queue->back->next = NULL;
	}
	node->next = cache->queue->front;
	node->prev = NULL;
	node->next->prev = node;
	cache->queue->front = node;
	pthread_rwlock_unlock(&cache->lock);
}

/* Return the value of the node at the front of the queue. */
buffer_t *peek(cache_t *cache) {
	return cache->queue->front->value;
}

/* Enqueue a node on the queue. */
void enqueue(queue_t *queue, node_t *node, pthread_rwlock_t lock) {
	pthread_rwlock_wrlock(&lock);
	node->next = queue->front;
	if (is_empty(queue)) {
		queue->back = node;
		queue->front = node;
	}
	else {
		queue->front->prev = node;
		queue->front = node;
	}
	queue->size++;
	pthread_rwlock_unlock(&lock);
}

/* Insert a node into the hash table and enqueue it. */
node_t *cache_insert(cache_t *cache, char *key, buffer_t *value) {
	pthread_rwlock_wrlock(&cache->lock);
	node_t *node = init_node(key, value);
	uint8_t hashed_key = hash(key);
	node_t *head = cache->hash_table->arr[(int)hashed_key];

	// if the buffer is too big, don't cache it
	if (buffer_length(value) >= MAX_OBJECT_SIZE) {
		return NULL;
	}
	
	// if the cache would be over capacity after the insertion, evict
	// repeat as necessary
	while (cache->size + buffer_length(value) >= MAX_CACHE_SIZE) { 
		buffer_t *removed_buffer = cache_remove(cache);
		cache->size -= buffer_length(removed_buffer);
	}

	if (head == NULL) {
		cache->hash_table->arr[hashed_key] = node;
	}
	else {
		// collision handling-- we create a chain
		node_t *head_prev;
		while (head != NULL) {
			head_prev = head;
			head = head->hash_table_next;
		}
		head_prev->hash_table_next = node;
	}
	cache->size += buffer_length(value);
	pthread_rwlock_unlock(&cache->lock);

	enqueue(cache->queue, node, cache->lock);
	return node;
}

buffer_t *cache_remove(cache_t *cache) {
	buffer_t *value = cache->queue->back->value;
	char *key_to_remove = cache->queue->back->key;
	uint8_t hashed_key = hash(key_to_remove);
	node_t *node = dequeue(cache->queue, cache->lock);
	(void)node;
	free(cache->hash_table->arr[hashed_key]);
	cache->hash_table->arr[hashed_key] = NULL;
	return value;
}

/*
 * If the cache contains the given value, return the corresponding node.
 * Else, return NULL.
 */
node_t *contains(cache_t *cache, char *key, pthread_rwlock_t lock) {
	pthread_rwlock_rdlock(&lock);

	uint8_t hashed_key = hash(key);
	node_t *node = cache->hash_table->arr[(int)hashed_key];
	while (node != NULL) {
		if (node->key == key) {
			pthread_rwlock_unlock(&lock);
			return node;
		}
		else {
			node = node->hash_table_next;
		}
	}
	pthread_rwlock_unlock(&lock);
	return node; // NULL in this case
}

pthread_rwlock_t get_lock(cache_t *cache) {
	return cache->lock;
}

queue_t *get_queue(cache_t *cache) {
	return cache->queue;
}

/* 
 * Iterate through the hash table, freeing each node in the linked list/chain
 * at each index (if it exists). Then, free the hash_table array and then
 * the struct.
 */
void free_hash_table(hash_table_t *hash_table) {
	for (size_t i = 0; i < hash_table->capacity; i++) {
		node_t *node_to_free = hash_table->arr[i];
		node_t *next_node_to_free = NULL;
		while (node_to_free != NULL) {
			next_node_to_free = node_to_free->hash_table_next;
			buffer_free(node_to_free->value);
			free(node_to_free);
			node_to_free = next_node_to_free;
		}
	}
	free(hash_table->arr);
	free(hash_table);
}

/* Free the hash table, queue, cache, and destroy the lock. */
void free_cache(cache_t *cache) {
	free_hash_table(cache->hash_table);
	free(cache->queue);
	pthread_rwlock_destroy(&cache->lock);
	free(cache);
}