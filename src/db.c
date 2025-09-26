#include "db.h"
#include <stdio.h>
#include <stdlib.h>

#define DB_FILE "rows.db"

static DbHeader header;

static void load_header(FILE* f) {
    fseek(f, 0, SEEK_SET);
    if (fread(&header, sizeof(DbHeader), 1, f) != 1) {
        // if empty file, init
        header.num_rows = 0;
        header.next_id = 1;
        fseek(f, 0, SEEK_SET);
        fwrite(&header, sizeof(DbHeader), 1, f);
        fflush(f);
    }
}

static void save_header(FILE* f) {
    fseek(f, 0, SEEK_SET);
    fwrite(&header, sizeof(DbHeader), 1, f);
    fflush(f);
}

void db_insert(Row* row) {
    FILE* f = fopen(DB_FILE, "r+b");
    if (!f) {
        f = fopen(DB_FILE, "w+b");
        if (!f) { perror("fopen"); exit(1); }
    }

    load_header(f);

    // assign unique autoincrement id
    row->id = header.next_id++;

    // write row at end (after header)
    fseek(f, sizeof(DbHeader) + header.num_rows * sizeof(Row), SEEK_SET);
    fwrite(row, sizeof(Row), 1, f);

    header.num_rows++;
    save_header(f);
    fclose(f);
}

void db_select_all() {
    FILE* f = fopen(DB_FILE, "rb");
    if (!f) { perror("fopen"); return; }

    load_header(f);

    Row r;
    for (int i = 0; i < header.num_rows; i++) {
        fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
        fread(&r, sizeof(Row), 1, f);
        printf("Row: id=%d, name=%s, age=%d\n", r.id, r.name, r.age);
    }
    fclose(f);
}

void db_select_by_id(int id) {
    FILE* f = fopen(DB_FILE, "rb");
    if (!f) { perror("fopen"); return; }

    load_header(f);

    Row r;
    for (int i = 0; i < header.num_rows; i++) {
        fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
        fread(&r, sizeof(Row), 1, f);
        if (r.id == id) {
            printf("Row: id=%d, name=%s, age=%d\n", r.id, r.name, r.age);
            fclose(f);
            return;
        }
    }
    printf("Row with id=%d not found.\n", id);
    fclose(f);
}