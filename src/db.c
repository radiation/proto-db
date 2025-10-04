#include "db.h"
#include "index.h"
#include "query.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static DbHeader header;
static Index id_index;
static Index name_index;
static const Row* update_values;

DbHeader* get_header(void) {
    return &header;
}

void load_header(FILE* f) {
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

void db_init() {
    index_init(&id_index, INDEX_INT, FIELD_ID, 16);
    index_init(&name_index, INDEX_STRING, FIELD_NAME, 16);

    FILE* f = fopen(DB_FILE, "rb");
    if (!f) return;

    load_header(f);

    Row r;
    for (int i = 0; i < header.num_rows; i++) {
        long offset = sizeof(DbHeader) + i * sizeof(Row);
        fseek(f, offset, SEEK_SET);
        fread(&r, sizeof(Row), 1, f);
        if (!r.is_deleted) {
            index_add(&id_index, &r.id, offset);
        }
    }
    fclose(f);
}

void db_insert(Row* row) {
    FILE* f = fopen(DB_FILE, "r+b");
    if (!f) {
        f = fopen(DB_FILE, "w+b");
        if (!f) { perror("fopen"); exit(1); }
    }

    load_header(f);

    row->id = header.next_id++;
    row->is_deleted = 0;

    long offset = sizeof(DbHeader) + header.num_rows * sizeof(Row);

    fseek(f, offset, SEEK_SET);
    fwrite(row, sizeof(Row), 1, f);
    fflush(f);

    header.num_rows++;
    save_header(f);

    index_add(&id_index, &row->id, offset);
    index_add(&name_index, row->name, offset);

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

        if (!r.is_deleted) {
            printf("Row: id=%d, name=%s, age=%d\n", r.id, r.name, r.age);
        }
    }
    fclose(f);
}

static void print_row(Row* r, long offset, FILE* f) {
    (void)offset;
    (void)f;
    printf("Row: id=%d, name=%s, age=%d\n", r->id, r->name, r->age);
}

void db_select_where(ConditionList* conds) {
    scan_rows(conds, print_row);
}

static void update_row(Row* r, long offset, FILE* f) {
    index_remove(&id_index, &r->id);
    index_remove(&name_index, r->name);

    if (update_values->age != -1) r->age = update_values->age;
    if (update_values->name[0] != '\0')
        strncpy(r->name, update_values->name, sizeof(r->name));

    fseek(f, offset, SEEK_SET);
    fwrite(r, sizeof(Row), 1, f);
    fflush(f);

    index_add(&id_index, &r->id, offset);
    index_add(&name_index, r->name, offset);
}

void db_update_where(ConditionList* conds, const Row* new_values) {
    update_values = new_values;
    scan_rows(conds, update_row);
    printf("Updated matching rows.\n");
}

void db_update_by_id(int id, const char* new_name, int new_age) {
    FILE* f = fopen(DB_FILE, "r+b");
    if (!f) { perror("fopen"); return; }

    load_header(f);

    Row r;
    for (int i = 0; i < header.num_rows; i++) {
        fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
        fread(&r, sizeof(Row), 1, f);

        if (r.id == id && !r.is_deleted) {
            if (strcmp(r.name, new_name) != 0 &&
                index_find(&name_index, new_name) != -1) {
                printf("Error: name '%s' already exists. Update rejected.\n", new_name);
                fclose(f);
                return;
            }

            strncpy(r.name, new_name, sizeof(r.name));
            r.age = new_age;

            fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
            fwrite(&r, sizeof(Row), 1, f);
            fflush(f);

            printf("Updated row with id=%d\n", id);
            fclose(f);
            return;
        }
    }

    printf("Row with id=%d not found or deleted.\n", id);
    fclose(f);
}

static void delete_row(Row* r, long offset, FILE* f) {
    r->is_deleted = 1;
    fseek(f, offset, SEEK_SET);
    fwrite(r, sizeof(Row), 1, f);
    fflush(f);

    index_remove(&id_index, &r->id);
    index_remove(&name_index, r->name);
}

void db_delete_where(ConditionList* conds) {
    scan_rows(conds, delete_row);
    printf("Deleted matching rows.\n");
}

void db_delete_by_id(int id) {
    FILE* f = fopen(DB_FILE, "r+b");
    if (!f) { perror("fopen"); return; }

    load_header(f);

    Row r;
    for (int i = 0; i < header.num_rows; i++) {
        fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
        fread(&r, sizeof(Row), 1, f);

        if (r.id == id && r.is_deleted == 0) {
            r.is_deleted = 1;
            fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
            fwrite(&r, sizeof(Row), 1, f);
            fflush(f);

            index_remove(&id_index, &r.id);
            index_remove(&name_index, r.name);

            printf("Deleted row with id=%d\n", id);
            fclose(f);
            return;
        }
    }

    printf("Row with id=%d not found or already deleted.\n", id);
    fclose(f);
}
