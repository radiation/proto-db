#ifndef CATALOG_H
#define CATALOG_H

#include "db.h"

#define MAX_TABLE_NAME 32
#define MAX_COLUMN_NAME 32
#define MAX_COLUMNS 16
#define MAX_TABLES 32
#define CATALOG_FILE "data/catalog.db"

// ---- Column and Table Definitions ----

typedef enum {
    COL_INT,
    COL_STRING
} ColumnType;

typedef struct {
    char name[MAX_COLUMN_NAME];
    ColumnType type;
    int length; // used only for strings
} ColumnDef;

typedef struct {
    char name[MAX_TABLE_NAME];
    int num_columns;
    ColumnDef columns[MAX_COLUMNS];
} TableDef;

typedef struct {
    int num_rows;
    int next_id;
} TableHeader;

// ---- Catalog API ----

/**
 * Initializes or loads the catalog into memory.
 * If the catalog file exists, loads it; otherwise creates an empty one.
 */
int catalog_init(void);

/**
 * Writes current in-memory catalog state to disk.
 */
int catalog_save(void);

/**
 * Adds a new table definition to the catalog and persists it.
 */
int catalog_create_table(const TableDef* def);

/**
 * Returns a pointer to a table definition by name.
 */
const TableDef* catalog_find_table(const char* name);

/**
 * Returns all tables currently in the catalog.
 */
int catalog_get_all(const TableDef** out_defs, int* out_count);

#endif
