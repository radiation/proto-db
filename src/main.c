#include <stdio.h>
#include <string.h>
#include "db.h"
#include "parser.h"

int main() {
    char command[256];
    printf("Welcome to MyDB! Commands: insert, select, select where id=N, exit\n");

    while (1) {
        printf("> ");
        if (!fgets(command, sizeof(command), stdin)) break;

        Command cmd = parse_command(command);

        switch (cmd.type) {
            case CMD_INSERT:
                db_insert(&cmd.row);
                printf("Inserted row.\n");
                break;
            case CMD_SELECT_ALL:
                db_select_all();
                break;
            case CMD_SELECT_ID:
                db_select_by_id(cmd.query_id);
                break;
            case CMD_EXIT:
                return 0;
            default:
                printf("Unknown command.\n");
        }
    }
    return 0;
}