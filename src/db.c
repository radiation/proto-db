#include "db.h"
#include "catalog.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_DIR "data"

void db_init(void) {
    struct stat st = {0};
    if (stat(DATA_DIR, &st) == -1) {
        mkdir(DATA_DIR, 0755);
        printf("Created data directory.\n");
    }

    // Initialize catalog (list of tables and columns)
    if (catalog_init()) {
        printf("Catalog initialized.\n");
    } else {
        fprintf(stderr, "Failed to initialize catalog.\n");
    }
}
