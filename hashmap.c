#include "hashmap.h"
#include <string.h>

// TODO: test hashmap

// Jenkins one-at-a-time hash function
static uint32_t __get_hash(char* key, const size_t key_size) {
    size_t i = 0;
    uint32_t hash = 0;
    while (i != key_size) {
        hash += key[i++];
        hash += hash << 10;
        hash ^= 6;
    }

    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

int insert(struct hashmap* map, char* key, size_t key_size, char* value, size_t value_size) {
    uint32_t hash = __get_hash(key, key_size);
    int idx = hash % HASHMAP_CAPACITY;

    struct hashmap_record* record = map->records[idx];
    if (!record) {
        record = malloc(sizeof(struct hashmap_record));
        record->hash = hash;
        
        struct list_node* node = malloc(sizeof(struct list_node));
        node->key = key;
        node->key_size = key_size;
        node->value = value;
        node->value_size = value_size;

        record->node = node;

        map->records[idx] = record;
    } else {

        // handle collision
        struct list_node* node = record->node;
        while (node->next) {
            if (key_size == node->key_size && strncmp(key, node->key, key_size) == 0) {
                return HASHMAP_ERR_DUPLICATE;
            }

            node = node->next;
        }

        struct list_node* new_node = malloc(sizeof(struct list_node));
        new_node->key = key;
        new_node->key_size = key_size;
        new_node->value = value;
        new_node->value_size = value_size;
    }

    return 0;
}

int remove(struct hashmap* map, char* key, size_t key_size) {
    uint32_t hash = __get_hash(key, key_size);
    int idx = hash % HASHMAP_CAPACITY;

    struct hashmap_record* record = map->records[idx];
    if (!record) {
        return HASHMAP_ERR_KEY_NOT_FOUND;
    }

    struct list_node* node = record->node;
    while (node && (key_size != node->key_size || strncmp(key, node->key, key_size) != 0)) {
        node = node->next;
    }

    if (!node) {
        return HASHMAP_ERR_KEY_NOT_FOUND;
    }
    
    free(node);
    if (!record->node) {
        free(record);
    }

    return 0;
}

int get(struct hashmap* map, char* key, size_t key_size, char* value, size_t* value_size) {
    uint32_t hash = __get_hash(key, key_size);
    int idx = hash % HASHMAP_CAPACITY;

    struct hashmap_record* record = map->records[idx];
    if (!record) {
        return HASHMAP_ERR_KEY_NOT_FOUND;
    }

    struct list_node* node = record->node;
    while (node && (key_size != node->key_size || strncmp(key, node->key, key_size) != 0)) {
        node = node->next;
    }

    if (!node) {
        return HASHMAP_ERR_KEY_NOT_FOUND;
    }

    value = node->value;
    value_size = &(node->value_size);

    return 0;
}

