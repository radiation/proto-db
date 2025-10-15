#include "catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static TableDef table_defs[MAX_TABLES];
static int table_count = 0;

// Ensure the data directory exists before writing catalog
static void ensure_data_dir(void) {
    struct stat st = {0};
    if (stat("data", &st) == -1) {
        mkdir("data", 0755);
    }
}

int catalog_init(void) {
    ensure_data_dir();

    FILE* f = fopen(CATALOG_FILE, "rb");
    if (!f) {
        // No catalog yet — initialize empty catalog
        table_count = 0;
        catalog_save();
        printf("Initialized empty catalog.\n");
        return 1;
    }

    table_count = 0;
    while (fread(&table_defs[table_count], sizeof(TableDef), 1, f) == 1) {
        table_count++;
        if (table_count >= MAX_TABLES) break;
    }

    fclose(f);
    printf("Loaded %d table definitions from catalog.\n", table_count);
    return 1;
}

int catalog_save(void) {
    ensure_data_dir();

    FILE* f = fopen(CATALOG_FILE, "wb");
    if (!f) {
        perror("fopen catalog_save");
        return 0;
    }

    fwrite(table_defs, sizeof(TableDef), table_count, f);
    fclose(f);
    return 1;
}

int catalog_create_table(const TableDef* def) {
    if (table_count >= MAX_TABLES) return 0;

    // Prevent duplicate table names
    for (int i = 0; i < table_count; i++) {
        if (strcmp(table_defs[i].name, def->name) == 0) {
            fprintf(stderr, "Error: Table '%s' already exists.\n", def->name);
            return 0;
        }
    }

    ensure_data_dir();

    // --- Write new definition to catalog.db ---
    FILE* f = fopen(CATALOG_FILE, "ab");
    if (!f) {
        perror("fopen catalog_create_table");
        return 0;
    }
    fwrite(def, sizeof(TableDef), 1, f);
    fclose(f);

    // Keep it in memory too
    table_defs[table_count++] = *def;

    // --- Create the corresponding table file ---
    char path[256];
    snprintf(path, sizeof(path), "data/%s.db", def->name);
    FILE* data = fopen(path, "wb");
    if (!data) {
        perror("fopen table file");
        return 0;
    }

    // Initialize table header (num_rows = 0, next_id = 1)
    TableHeader header = {0, 1};
    fwrite(&header, sizeof(TableHeader), 1, data);
    fclose(data);

    printf("Table '%s' added to catalog and data file created.\n", def->name);
    return 1;
}

const TableDef* catalog_find_table(const char* name) {
    for (int i = 0; i < table_count; i++) {
        if (strcmp(table_defs[i].name, name) == 0) {
            return &table_defs[i];
        }
    }
    return NULL;
}

int catalog_get_all(const TableDef** out_defs, int* out_count) {
    *out_defs = table_defs;
    *out_count = table_count;
    return 1;
}
