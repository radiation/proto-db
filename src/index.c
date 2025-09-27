#include "index.h"
#include <stdlib.h>
#include <string.h>
#include "db.h"

void index_init(Index* idx, int capacity) {
    idx->entries = malloc(sizeof(IndexEntry) * capacity);
    idx->size = 0;
    idx->capacity = capacity;
}

void index_free(Index* idx) {
    free(idx->entries);
    idx->entries = NULL;
    idx->size = 0;
    idx->capacity = 0;
}

void index_add(Index* idx, int id, long offset) {
    // grow if full
    if (idx->size >= idx->capacity) {
        idx->capacity *= 2;
        idx->entries = realloc(idx->entries, sizeof(IndexEntry) * idx->capacity);
    }
    idx->entries[idx->size].id = id;
    idx->entries[idx->size].offset = offset;
    idx->size++;
}

long index_find(Index* idx, int id) {
    for (int i = 0; i < idx->size; i++) {
        if (idx->entries[i].id == id) {
            return idx->entries[i].offset;
        }
    }
    return -1;  // not found
}
