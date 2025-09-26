#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "db.h"

int main() {
    char command[256];
    printf("Welcome to MyDB! Enter commands (insert/select/exit)\n");

    while (1) {
        printf("> ");
        if (!fgets(command, sizeof(command), stdin)) break;

        if (strncmp(command, "insert", 6) == 0) {
            Row r;
            if (sscanf(command, "insert %d %31s %d", &r.id, r.name, &r.age) == 3) {
                db_insert(&r);
                printf("Inserted row.\n");
            } else {
                printf("Usage: insert <id> <name> <age>\n");
            }
        } else if (strncmp(command, "select", 6) == 0) {
            db_select_all();
        } else if (strncmp(command, "exit", 4) == 0) {
            break;
        } else {
            printf("Unknown command.\n");
        }
    }

    return 0;
}
