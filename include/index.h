typedef struct {
    int id;
    long offset;
} IndexEntry;

typedef struct {
    IndexEntry* entries;
    int size;
    int capacity;
} Index;

void index_init(Index* idx, int capacity);
void index_free(Index* idx);
void index_add(Index* idx, int id, long offset);
long index_find(Index* idx, int id);
