#include "db.h"
#include <stdio.h>
#include <stdlib.h>

#define DB_FILE "rows.db"

void db_insert(const Row* row) {
    FILE* f = fopen(DB_FILE, "ab"); // append binary
    if (!f) {
        perror("fopen");
        exit(1);
    }
    fwrite(row, sizeof(Row), 1, f);
    fclose(f);
}

void db_select_all() {
    FILE* f = fopen(DB_FILE, "rb"); // read binary
    if (!f) {
        perror("fopen");
        return;
    }
    Row r;
    while (fread(&r, sizeof(Row), 1, f)) {
        printf("Row: id=%d, name=%s, age=%d\n", r.id, r.name, r.age);
    }
    fclose(f);
}
