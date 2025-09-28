#include "db.h"
#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DB_FILE "rows.db"

static DbHeader header;
static Index id_index;

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

static int unique_field_exists(FILE* f, Field field, const void* value, int exclude_id) {
    Row r;
    for (int i = 0; i < header.num_rows; i++) {
        fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
        fread(&r, sizeof(Row), 1, f);
        if (r.is_deleted) continue;

        // Skip the row being updated
        if (exclude_id != -1 && r.id == exclude_id) continue;

        switch (field) {
            case FIELD_ID:
                if (r.id == *(int*)value) return 1;
                break;
            case FIELD_NAME:
                if (strncmp(r.name, (char*)value, sizeof(r.name)) == 0) return 1;
                break;
            case FIELD_AGE:
                if (r.age == *(int*)value) return 1;
                break;
        }
    }
    return 0;
}

void db_init() {
    index_init(&id_index, 16);

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

    if (unique_field_exists(f, FIELD_NAME, row->name, -1)) {
        printf("Error: name '%s' already exists.\n", row->name);
        fclose(f);
        return;
    }

    load_header(f);

    row->id = header.next_id++;

    fseek(f, sizeof(DbHeader) + header.num_rows * sizeof(Row), SEEK_SET);
    fwrite(row, sizeof(Row), 1, f);

    header.num_rows++;
    save_header(f);
    fclose(f);

    long offset = sizeof(DbHeader) + (header.num_rows - 1) * sizeof(Row);
    index_add(&id_index, &row->id, offset);
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

void db_select_where(Field field, Operator op, const char* str_val, int int_val) {
    FILE* f = fopen(DB_FILE, "rb");
    if (!f) { perror("fopen"); return; }

    load_header(f);

    Row r;
    for (int i = 0; i < header.num_rows; i++) {
        fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
        fread(&r, sizeof(Row), 1, f);
        if (r.is_deleted) continue;

        int match = 0;
        switch (field) {
            case FIELD_ID:
                if ((op == OP_EQ && r.id == int_val) ||
                    (op == OP_GT && r.id > int_val) ||
                    (op == OP_LT && r.id < int_val)) match = 1;
                break;
            case FIELD_AGE:
                if ((op == OP_EQ && r.age == int_val) ||
                    (op == OP_GT && r.age > int_val) ||
                    (op == OP_LT && r.age < int_val)) match = 1;
                break;
            case FIELD_NAME:
                if ((op == OP_EQ && strncmp(r.name, str_val, sizeof(r.name)) == 0)) {
                    match = 1;
                }
                // We’ll skip > and < for strings for now
                break;
        }

        if (match) {
            printf("Row: id=%d, name=%s, age=%d\n", r.id, r.name, r.age);
        }
    }
    fclose(f);
}

void db_update_by_id(int id, const char* new_name, int new_age) {
    FILE* f = fopen(DB_FILE, "r+b");
    if (!f) { perror("fopen"); return; }

    if (unique_field_exists(f, FIELD_NAME, new_name, id)) {
        printf("Error: name '%s' already exists. Update rejected.\n", new_name);
        fclose(f);
        return;
    }

    load_header(f);

    Row r;
    for (int i = 0; i < header.num_rows; i++) {
        fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
        fread(&r, sizeof(Row), 1, f);

        if (r.id == id && !r.is_deleted) {
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

void db_delete_by_id(int id) {
    FILE* f = fopen(DB_FILE, "r+b");
    if (!f) { perror("fopen"); return; }

    load_header(f);

    Row r;
    for (int i = 0; i < header.num_rows; i++) {
        fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
        fread(&r, sizeof(Row), 1, f);

        if (r.id == id && r.is_deleted == 0) {
            r.is_deleted = 1;  // mark as deleted
            fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
            fwrite(&r, sizeof(Row), 1, f);
            fflush(f);
            printf("Deleted row with id=%d\n", id);
            fclose(f);
            return;
        }
    }

    printf("Row with id=%d not found or already deleted.\n", id);
    fclose(f);
}
