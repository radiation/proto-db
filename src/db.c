#include "db.h"
#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DB_FILE "rows.db"

static DbHeader header;
static Index id_index;
static Index name_index;

static int eval_condition(Row* r, Condition* cond);

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

void db_select_where(Field field, Operator op, const char* str_val, int int_val) {
    FILE* f = fopen(DB_FILE, "rb");
    if (!f) { perror("fopen"); return; }

    load_header(f);
    Row r;

    // Case 1: exact match on indexed fields (id or name)
    if (op == OP_EQ && field == FIELD_ID) {
        long offset = index_find(&id_index, &int_val);
        if (offset != -1) {
            fseek(f, offset, SEEK_SET);
            fread(&r, sizeof(Row), 1, f);
            if (!r.is_deleted) {
                printf("Row: id=%d, name=%s, age=%d\n", r.id, r.name, r.age);
            }
        } else {
            printf("Row with id=%d not found.\n", int_val);
        }
        fclose(f);
        return;
    }

    if (op == OP_EQ && field == FIELD_NAME) {
        long offset = index_find(&name_index, str_val);
        if (offset != -1) {
            fseek(f, offset, SEEK_SET);
            fread(&r, sizeof(Row), 1, f);
            if (!r.is_deleted) {
                printf("Row: id=%d, name=%s, age=%d\n", r.id, r.name, r.age);
            }
        } else {
            printf("Row with name='%s' not found.\n", str_val);
        }
        fclose(f);
        return;
    }

    // Case 2: fallback to scanning for everything else
    for (int i = 0; i < header.num_rows; i++) {
        fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
        fread(&r, sizeof(Row), 1, f);
        if (r.is_deleted) continue;

        int match = 0;
        switch (field) {
            case FIELD_ID:
                if ((op == OP_GT && r.id > int_val) ||
                    (op == OP_LT && r.id < int_val)) match = 1;
                break;
            case FIELD_AGE:
                if ((op == OP_EQ && r.age == int_val) ||
                    (op == OP_GT && r.age > int_val) ||
                    (op == OP_LT && r.age < int_val)) match = 1;
                break;
            case FIELD_NAME:
                if (op == OP_EQ && strcmp(r.name, str_val) == 0) match = 1;
                break;
        }

        if (match) {
            printf("Row: id=%d, name=%s, age=%d\n", r.id, r.name, r.age);
        }
    }
    fclose(f);
}

void db_select_where_list(ConditionList* conds) {
    FILE* f = fopen(DB_FILE, "rb");
    if (!f) { perror("fopen"); return; }

    load_header(f);
    Row r;

    for (int i = 0; i < header.num_rows; i++) {
        fseek(f, sizeof(DbHeader) + i * sizeof(Row), SEEK_SET);
        fread(&r, sizeof(Row), 1, f);
        if (r.is_deleted) continue;

        int result = eval_condition(&r, &conds->conds[0]);

        for (int j = 0; j < conds->op_count; j++) {
            int rhs = eval_condition(&r, &conds->conds[j + 1]);
            if (conds->ops[j] == LOGICAL_AND) result = result && rhs;
            else if (conds->ops[j] == LOGICAL_OR) result = result || rhs;
        }

        if (result) {
            printf("Row: id=%d, name=%s, age=%d\n", r.id, r.name, r.age);
        }
    }

    fclose(f);
}

void db_update_where(ConditionList* conds, const Row* new_values) {
    FILE* f = fopen(DB_FILE, "r+b");
    if (!f) { perror("fopen"); return; }

    load_header(f);
    Row r;
    int updated_count = 0;

    for (int i = 0; i < header.num_rows; i++) {
        long offset = sizeof(DbHeader) + i * sizeof(Row);
        fseek(f, offset, SEEK_SET);
        fread(&r, sizeof(Row), 1, f);
        if (r.is_deleted) continue;

        // Evaluate conditions
        int result = eval_condition(&r, &conds->conds[0]);
        for (int j = 0; j < conds->op_count; j++) {
            int rhs = eval_condition(&r, &conds->conds[j + 1]);
            if (conds->ops[j] == LOGICAL_AND) result = result && rhs;
            else if (conds->ops[j] == LOGICAL_OR) result = result || rhs;
        }

        if (result) {
            // Remove old index entries if needed
            index_remove(&name_index, r.name);
            index_remove(&id_index, &r.id);

            // Apply updates
            if (new_values->age != -1) r.age = new_values->age;
            if (new_values->name[0] != '\0') strncpy(r.name, new_values->name, sizeof(r.name));

            // Rewrite row
            fseek(f, offset, SEEK_SET);
            fwrite(&r, sizeof(Row), 1, f);
            fflush(f);

            // Re-index updated values
            index_add(&id_index, &r.id, offset);
            index_add(&name_index, r.name, offset);

            updated_count++;
        }
    }

    fclose(f);
    printf("Updated %d matching rows.\n", updated_count);
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

void db_delete_where(ConditionList* conds) {
    FILE* f = fopen(DB_FILE, "r+b");
    if (!f) { perror("fopen"); return; }

    load_header(f);

    Row r;
    int deleted_count = 0;

    for (int i = 0; i < header.num_rows; i++) {
        long offset = sizeof(DbHeader) + i * sizeof(Row);
        fseek(f, offset, SEEK_SET);
        fread(&r, sizeof(Row), 1, f);
        if (r.is_deleted) continue;

        int result = eval_condition(&r, &conds->conds[0]);
        for (int j = 0; j < conds->op_count; j++) {
            int rhs = eval_condition(&r, &conds->conds[j + 1]);
            if (conds->ops[j] == LOGICAL_AND) result = result && rhs;
            else if (conds->ops[j] == LOGICAL_OR) result = result || rhs;
        }

        if (result) {
            r.is_deleted = 1;
            fseek(f, offset, SEEK_SET);
            fwrite(&r, sizeof(Row), 1, f);
            fflush(f);

            index_remove(&id_index, &r.id);
            index_remove(&name_index, r.name);

            deleted_count++;
        }
    }

    fclose(f);
    printf("Deleted %d matching rows.\n", deleted_count);
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

static int eval_condition(Row* r, Condition* cond) {
    switch (cond->field) {
        case FIELD_ID:
            if (cond->op == OP_EQ) return r->id == cond->int_value;
            if (cond->op == OP_GT) return r->id > cond->int_value;
            if (cond->op == OP_LT) return r->id < cond->int_value;
            break;
        case FIELD_AGE:
            if (cond->op == OP_EQ) return r->age == cond->int_value;
            if (cond->op == OP_GT) return r->age > cond->int_value;
            if (cond->op == OP_LT) return r->age < cond->int_value;
            break;
        case FIELD_NAME:
            if (cond->op == OP_EQ) return strcmp(r->name, cond->str_value) == 0;
            break;
    }
    return 0;
}
