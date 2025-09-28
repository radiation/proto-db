#include "db.h"

typedef struct {
    int id;
    long offset;
} IndexEntry;

typedef enum {
    INDEX_INT,
    INDEX_STRING
} IndexType;

typedef struct {
    IndexType type;
    Field field;
    void* keys;
    long* offsets;
    int size;
    int capacity;
} Index;

void index_init(Index* idx, IndexType type, Field field, int capacity);
void index_free(Index* idx);
void index_add(Index* idx, const void* key, long offset);
long index_find(Index* idx, const void* key);
