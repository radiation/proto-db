#include "index.h"
#include <stdlib.h>
#include <string.h>
#include "db.h"

void index_init(Index* idx, IndexType type, Field field, int capacity) {
    idx->type = type;
    idx->field = field;
    idx->size = 0;
    idx->capacity = capacity;

    if (type == INDEX_INT) {
        idx->keys = malloc(sizeof(int) * capacity);
    } else if (type == INDEX_STRING) {
        idx->keys = malloc(sizeof(char*) * capacity);
    }

    idx->offsets = malloc(sizeof(long) * capacity);
}

void index_free(Index* idx) {
    if (idx->type == INDEX_STRING && idx->keys) {
        char** arr = (char**)idx->keys;
        for (int i = 0; i < idx->size; i++) {
            free(arr[i]);  // free copies of strings
        }
    }
    free(idx->keys);
    free(idx->offsets);

    idx->keys = NULL;
    idx->offsets = NULL;
    idx->size = 0;
    idx->capacity = 0;
}

void index_add(Index* idx, const void* key, long offset) {
    if (idx->size >= idx->capacity) {
        idx->capacity *= 2;
        if (idx->type == INDEX_INT) {
            idx->keys = realloc(idx->keys, sizeof(int) * idx->capacity);
        } else {
            idx->keys = realloc(idx->keys, sizeof(char*) * idx->capacity);
        }
        idx->offsets = realloc(idx->offsets, sizeof(long) * idx->capacity);
    }

    if (idx->type == INDEX_INT) {
        int* arr = (int*)idx->keys;
        arr[idx->size] = *(int*)key;
    } else if (idx->type == INDEX_STRING) {
        const char* str = (const char*)key;
        char** arr = (char**)idx->keys;
        arr[idx->size] = strdup(str); // make a copy
    }

    idx->offsets[idx->size] = offset;
    idx->size++;
}

void index_remove(Index* idx, const void* key) {
    if (idx->type == INDEX_INT) {
        int target = *(int*)key;
        int* arr = (int*)idx->keys;

        for (int i = 0; i < idx->size; i++) {
            if (arr[i] == target) {
                // shift everything down
                for (int j = i; j < idx->size - 1; j++) {
                    arr[j] = arr[j + 1];
                    idx->offsets[j] = idx->offsets[j + 1];
                }
                idx->size--;
                return;
            }
        }
    } else if (idx->type == INDEX_STRING) {
        const char* target = (const char*)key;
        char** arr = (char**)idx->keys;

        for (int i = 0; i < idx->size; i++) {
            if (strcmp(arr[i], target) == 0) {
                free(arr[i]); // free string copy
                for (int j = i; j < idx->size - 1; j++) {
                    arr[j] = arr[j + 1];
                    idx->offsets[j] = idx->offsets[j + 1];
                }
                idx->size--;
                return;
            }
        }
    }
}

long index_find(Index* idx, const void* key) {
    if (idx->type == INDEX_INT) {
        int target = *(int*)key;
        int* arr = (int*)idx->keys;
        for (int i = 0; i < idx->size; i++) {
            if (arr[i] == target) return idx->offsets[i];
        }
    } else if (idx->type == INDEX_STRING) {
        const char* target = (const char*)key;
        char** arr = (char**)idx->keys;
        for (int i = 0; i < idx->size; i++) {
            if (strcmp(arr[i], target) == 0) return idx->offsets[i];
        }
    }
    return -1;  // not found
}
