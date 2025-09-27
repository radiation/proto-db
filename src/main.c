#include <stdio.h>
#include <string.h>
#include "db.h"
#include "parser.h"

int main() {
    db_init();

    char command[256];
    printf("Welcome to ProtoDB! Commands: insert, select, select where id=N, exit\n");

    while (1) {
        printf("> ");
        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = 0;

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
            case CMD_UPDATE:
                db_update_by_id(cmd.query_id, cmd.row.name, cmd.row.age);
                break;
            case CMD_DELETE:
                db_delete_by_id(cmd.query_id);
                break;
            case CMD_EXIT:
                return 0;
            default:
                printf("Unknown command.\n");
        }
    }
    return 0;
}