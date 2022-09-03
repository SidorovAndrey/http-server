#include <stdlib.h>

#define HASHMAP_CAPACITY 1024
#define HASHMAP_ERR_DUPLICATE 1
#define HASHMAP_ERR_KEY_NOT_FOUND 2

//struct list_node;

struct list_node {
    char* key;
    size_t key_size;
    char* value;
    size_t value_size;
    struct list_node* next;
};

struct hashmap_record {
    uint32_t hash;
    struct list_node* node;
};

struct hashmap {
    struct hashmap_record* records[HASHMAP_CAPACITY];
};

int insert(struct hashmap* map, char* key, size_t key_size, char* value, size_t value_size);
int remove(struct hashmap* map, char* key, size_t key_size);
int get(struct hashmap* map, char* key, size_t key_size, char* value, size_t* value_size);

