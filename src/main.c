#include <stdio.h>
#include <string.h>
#include "catalog.h"
#include "command.h"
#include "db.h"
#include "parser.h"

int main() {
    db_init();

    char command[256];
    printf("Welcome to ProtoDB! Do normal DB stuff or type exit\n");

    while (1) {
        printf("> ");
        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = 0;

        Command cmd = parse_command(command);
        printf("Parsed command of type %d\n", cmd.type);
        int rc = db_execute_command(&cmd);

        if (rc == DB_EXIT || cmd.type == CMD_EXIT)
            break;

        if (rc == DB_ERROR)
            fprintf(stderr, "Command failed or is not implemented.\n");

        fflush(stdout);
    }
    return 0;
}