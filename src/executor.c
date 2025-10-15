#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "catalog.h"
#include "executor.h"
#include "query.h"

#define DATA_DIR "data"
#define ROW_BUFFER_SIZE 1024

static void ensure_data_dir_exists() {
    struct stat st = {0};
    if (stat(DATA_DIR, &st) == -1) {
        mkdir(DATA_DIR, 0755);
    }
}

static int read_row(FILE* f, const TableDef* def, ColumnValue* out_values) {
    for (int i = 0; i < def->num_columns; i++) {
        strncpy(out_values[i].column, def->columns[i].name, sizeof(out_values[i].column));

        if (def->columns[i].type == COL_INT) {
            out_values[i].type = VALUE_INT;
            fread(&out_values[i].int_val, sizeof(int), 1, f);
        } else if (def->columns[i].type == COL_STRING) {
            out_values[i].type = VALUE_STRING;
            fread(out_values[i].str_val, def->columns[i].length, 1, f);
            out_values[i].str_val[def->columns[i].length - 1] = '\0';  // safety
        }
    }
    return 1;
}

static void write_column(FILE* f, ColumnType type, int length, ColumnValue* val) {
    if (type == COL_INT) {
        fwrite(&val->int_val, sizeof(int), 1, f);
    } else if (type == COL_STRING) {
        char buffer[64] = {0};
        strncpy(buffer, val->str_val, length);
        fwrite(buffer, length, 1, f);
    }
}

int db_insert_row(const char* table_name, ColumnValue* values, int count) {
    const TableDef* def = catalog_find_table(table_name);
    if (!def) {
        fprintf(stderr, "Insert error: table '%s' not found.\n", table_name);
        return -1;
    }

    // Prepare data directory and file
    ensure_data_dir_exists();
    char path[256];
    snprintf(path, sizeof(path), "data/%s.db", table_name);

    FILE* f = fopen(path, "r+b");
    if (!f) {
        // Create and initialize header if file doesn't exist
        f = fopen(path, "w+b");
        if (!f) {
            perror("fopen");
            return -1;
        }
        TableHeader header = {0, 1};
        fwrite(&header, sizeof(TableHeader), 1, f);
        fflush(f);
    }

    // Read header
    TableHeader header;
    fseek(f, 0, SEEK_SET);
    fread(&header, sizeof(TableHeader), 1, f);

    // Prepare a complete row array matching schema
    ColumnValue full_row[MAX_COLUMNS];
    int expected = def->num_columns;

    // Build the full row based on schema + provided values
    for (int i = 0; i < expected; i++) {
        const ColumnDef* col = &def->columns[i];
        ColumnValue* v = &full_row[i];
        strncpy(v->column, col->name, sizeof(v->column));

        // Auto-increment ID if applicable
        if (strcmp(col->name, "id") == 0 && col->type == COL_INT) {
            v->type = VALUE_INT;
            v->int_val = header.next_id++;
            continue;
        }

        // Try to find a matching user-specified value by column name
        int matched = 0;
        for (int j = 0; j < count; j++) {
            if (strcmp(values[j].column, col->name) == 0) {
                v->type = values[j].type;
                if (v->type == VALUE_INT)
                    v->int_val = values[j].int_val;
                else
                    strncpy(v->str_val, values[j].str_val, sizeof(v->str_val));
                matched = 1;
                break;
            }
        }

        // Fill defaults for any missing columns
        if (!matched) {
            v->type = (col->type == COL_INT) ? VALUE_INT : VALUE_STRING;
            if (col->type == COL_INT)
                v->int_val = 0;
            else
                v->str_val[0] = '\0';
        }
    }

    // Seek to the end of the table and write the row
    long offset = sizeof(TableHeader) + header.num_rows * ROW_BUFFER_SIZE;
    fseek(f, offset, SEEK_SET);

    for (int i = 0; i < expected; i++) {
        if (full_row[i].type == VALUE_INT)
            fwrite(&full_row[i].int_val, sizeof(int), 1, f);
        else
            fwrite(full_row[i].str_val, def->columns[i].length, 1, f);
    }

    header.num_rows++;
    fseek(f, 0, SEEK_SET);
    fwrite(&header, sizeof(TableHeader), 1, f);
    fflush(f);
    fclose(f);

    printf("Inserted row into '%s' (id=%d).\n", table_name, header.next_id - 1);
    return 0;
}

int db_select_all(const char* table_name) {
    const TableDef* def = catalog_find_table(table_name);
    if (!def) {
        fprintf(stderr, "Select error: table '%s' not found.\n", table_name);
        return -1;
    }

    char path[256];
    snprintf(path, sizeof(path), "data/%s.db", table_name);

    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Table file '%s' not found.\n", path);
        return -1;
    }

    TableHeader header;
    fread(&header, sizeof(TableHeader), 1, f);

    printf("Rows in '%s' (%d rows):\n", table_name, header.num_rows);

    for (int i = 0; i < header.num_rows; i++) {
        long offset = sizeof(TableHeader) + i * ROW_BUFFER_SIZE;
        fseek(f, offset, SEEK_SET);

        printf("- ");
        for (int j = 0; j < def->num_columns; j++) {
            ColumnDef col = def->columns[j];

            if (col.type == COL_INT) {
                int val = 0;
                fread(&val, sizeof(int), 1, f);
                printf("%s=%d ", col.name, val);
            } else if (col.type == COL_STRING) {
                char buffer[64] = {0};
                fread(buffer, col.length, 1, f);
                buffer[col.length - 1] = '\0';  // safety null-term
                printf("%s=\"%s\" ", col.name, buffer);
            }
        }

        printf("\n");
    }

    fclose(f);
    return 0;
}

int db_select_where(const char* table_name, const ConditionList* conds) {
    const TableDef* def = catalog_find_table(table_name);
    if (!def) {
        fprintf(stderr, "Select error: table '%s' not found.\n", table_name);
        return -1;
    }

    char path[256];
    snprintf(path, sizeof(path), "data/%s.db", table_name);
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Table file '%s' not found.\n", path);
        return -1;
    }

    TableHeader header;
    fread(&header, sizeof(TableHeader), 1, f);

    ColumnValue values[MAX_COLUMNS];
    int match_count = 0;

    for (int i = 0; i < header.num_rows; i++) {
        long offset = sizeof(TableHeader) + i * ROW_BUFFER_SIZE;
        fseek(f, offset, SEEK_SET);

        read_row(f, def, values);

        if (eval_condition_list(values, def->num_columns, conds)) {
            match_count++;
            printf("- ");
            for (int j = 0; j < def->num_columns; j++) {
                if (values[j].type == VALUE_INT)
                    printf("%s=%d ", values[j].column, values[j].int_val);
                else
                    printf("%s=\"%s\" ", values[j].column, values[j].str_val);
            }
            printf("\n");
        }
    }

    fclose(f);
    printf("Matched %d row(s).\n", match_count);
    return 0;
}

// ---------------------------------------------------------
// UPDATE WHERE
// ---------------------------------------------------------
int db_update_where(const char* table_name,
                    const ConditionList* conds,
                    const ColumnValue* updates,
                    int update_count) {
    const TableDef* def = catalog_find_table(table_name);
    if (!def) {
        fprintf(stderr, "Error: table '%s' not found.\n", table_name);
        return -1;
    }

    char path[256];
    snprintf(path, sizeof(path), "data/%s.db", table_name);

    FILE* f = fopen(path, "r+b");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s'\n", path);
        return -1;
    }

    TableHeader header;
    fread(&header, sizeof(TableHeader), 1, f);

    ColumnValue row[MAX_COLUMNS];
    int updated = 0;

    for (int i = 0; i < header.num_rows; i++) {
        long offset = sizeof(TableHeader) + i * ROW_BUFFER_SIZE;
        fseek(f, offset, SEEK_SET);

        // --- Read the row into memory ---
        for (int j = 0; j < def->num_columns; j++) {
            const ColumnDef* col = &def->columns[j];
            ColumnValue* val = &row[j];
            strncpy(val->column, col->name, sizeof(val->column));
            val->type = (col->type == COL_INT) ? VALUE_INT : VALUE_STRING;

            if (col->type == COL_INT) {
                fread(&val->int_val, sizeof(int), 1, f);
            } else {
                fread(val->str_val, col->length, 1, f);
                val->str_val[col->length - 1] = '\0';
            }
        }

        // --- Evaluate WHERE conditions ---
        if (eval_condition_list(row, def->num_columns, conds)) {
            // --- Apply updates by column name ---
            for (int u = 0; u < update_count; u++) {
                for (int j = 0; j < def->num_columns; j++) {
                    if (strcmp(row[j].column, updates[u].column) == 0) {
                        if (updates[u].type == VALUE_INT)
                            row[j].int_val = updates[u].int_val;
                        else
                            strncpy(row[j].str_val, updates[u].str_val,
                                    sizeof(row[j].str_val));
                        break; // Once we’ve updated, stop inner loop
                    }
                }
            }

            // --- Seek back & overwrite this row ---
            fseek(f, offset, SEEK_SET);
            for (int j = 0; j < def->num_columns; j++) {
                if (row[j].type == VALUE_INT)
                    fwrite(&row[j].int_val, sizeof(int), 1, f);
                else
                    fwrite(row[j].str_val, def->columns[j].length, 1, f);
            }

            fflush(f); // ensure the change is written immediately
            updated++;
        }
    }

    // --- rewrite header ---
    fseek(f, 0, SEEK_SET);
    fwrite(&header, sizeof(TableHeader), 1, f);
    fflush(f);

    fclose(f);
    printf("Updated %d row(s) in '%s'.\n", updated, table_name);

    // Return the number of rows actually updated (not 0)
    return updated;
}

// ---------------------------------------------------------
// DELETE WHERE
// ---------------------------------------------------------
int db_delete_where(const char* table_name, const ConditionList* conds) {
    const TableDef* def = catalog_find_table(table_name);
    if (!def) {
        fprintf(stderr, "Error: table '%s' not found.\n", table_name);
        return -1;
    }

    char path[256], temp_path[256];
    snprintf(path, sizeof(path), "data/%s.db", table_name);
    snprintf(temp_path, sizeof(temp_path), "data/%s.tmp", table_name);

    FILE* in = fopen(path, "rb");
    if (!in) {
        fprintf(stderr, "Error: cannot open '%s'\n", path);
        return -1;
    }

    FILE* out = fopen(temp_path, "wb");
    if (!out) {
        perror("fopen temp file");
        fclose(in);
        return -1;
    }

    TableHeader header;
    fread(&header, sizeof(TableHeader), 1, in);
    TableHeader new_header = {0, header.next_id};
    fwrite(&new_header, sizeof(TableHeader), 1, out);

    ColumnValue row[MAX_COLUMNS];
    int deleted = 0;

    for (int i = 0; i < header.num_rows; i++) {
        long offset = sizeof(TableHeader) + i * ROW_BUFFER_SIZE;
        fseek(in, offset, SEEK_SET);

        // Deserialize row
        for (int j = 0; j < def->num_columns; j++) {
            ColumnDef col = def->columns[j];
            ColumnValue* val = &row[j];
            strncpy(val->column, col.name, sizeof(val->column));
            val->type = (col.type == COL_INT) ? VALUE_INT : VALUE_STRING;

            if (col.type == COL_INT) {
                fread(&val->int_val, sizeof(int), 1, in);
            } else {
                fread(val->str_val, col.length, 1, in);
                val->str_val[col.length - 1] = '\0';
            }
        }

        // Evaluate conditions
        if (eval_condition_list(row, def->num_columns, conds)) {
            deleted++;
            continue; // skip writing deleted row
        }

        // Write row back to temp file
        for (int j = 0; j < def->num_columns; j++) {
            if (row[j].type == VALUE_INT)
                fwrite(&row[j].int_val, sizeof(int), 1, out);
            else
                fwrite(row[j].str_val, def->columns[j].length, 1, out);
        }

        new_header.num_rows++;
    }

    // Rewrite header with updated row count
    fseek(out, 0, SEEK_SET);
    fwrite(&new_header, sizeof(TableHeader), 1, out);

    fclose(in);
    fclose(out);

    // Replace old file with new compacted one
    remove(path);
    rename(temp_path, path);

    printf("Deleted %d row(s) from '%s'.\n", deleted, table_name);
    return 0;
}

int db_execute_command(const Command* cmd) {
    switch (cmd->type) {
        case CMD_CREATE_TABLE: {
            TableDef def;
            strncpy(def.name, cmd->create_table.table_name, sizeof(def.name));
            def.num_columns = cmd->create_table.column_count;
            memcpy(def.columns, cmd->create_table.columns,
                   sizeof(ColumnDef) * def.num_columns);

            if (catalog_create_table(&def)) {
                printf("Table '%s' created successfully.\n", def.name);
            } else {
                fprintf(stderr, "Error creating table '%s'.\n", def.name);
            }
            return 0;
        }

        case CMD_DESCRIBE: {
            const char* name = cmd->describe.table_name;
            const TableDef* def = catalog_find_table(name);

            if (!def) {
                fprintf(stderr, "Error: table '%s' not found.\n", name);
                return -1;
            }

            printf("Table: %s\n", def->name);
            printf("Columns:\n");
            for (int i = 0; i < def->num_columns; i++) {
                const ColumnDef* col = &def->columns[i];
                const char* type_str = (col->type == COL_INT) ? "int" : "string";
                printf("- %s (%s)\n", col->name, type_str);
            }
            return 0;
        }

        case CMD_INSERT:
            return db_insert_row(cmd->insert.table_name,
                                 cmd->insert.values,
                                 cmd->insert.value_count);

        case CMD_SELECT_ALL:
            return db_select_all(cmd->select_all.table_name);

        case CMD_SELECT_COND:
            return db_select_where(cmd->select_where.table_name,
                                   &cmd->select_where.conds);

        case CMD_UPDATE_WHERE:
            return db_update_where(cmd->update_where.table_name,
                                   &cmd->update_where.conds,
                                   cmd->update_where.updates,
                                   cmd->update_where.update_count);

        case CMD_DELETE_WHERE:
            return db_delete_where(cmd->delete_where.table_name,
                                   &cmd->delete_where.conds);

        case CMD_EXIT:
            printf("Exiting ProtoDB.\n");
            return 1;

        default:
            fprintf(stderr, "Unknown or unimplemented command type (%d).\n",
                    cmd->type);
            return -1;
    }
}
