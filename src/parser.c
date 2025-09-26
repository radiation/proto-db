#include "parser.h"
#include <stdio.h>
#include <string.h>

Command parse_command(const char* input) {
    Command cmd;
    cmd.type = CMD_UNKNOWN;

    if (strncmp(input, "insert", 6) == 0) {
        Row r;
        if (sscanf(input, "insert %31s %d", r.name, &r.age) == 2) {
            cmd.type = CMD_INSERT;
            cmd.row = r;
        }
    } else if (strncmp(input, "select where id", 15) == 0) {
        int id;
        if (sscanf(input, "select where id = %d", &id) == 1) {
            cmd.type = CMD_SELECT_ID;
            cmd.query_id = id;
        }
    } else if (strncmp(input, "select", 6) == 0) {
        cmd.type = CMD_SELECT_ALL;
    } else if (strncmp(input, "update", 6) == 0) {
        int id, age;
        char name[32];
        if (sscanf(input, "update %d %31s %d", &id, name, &age) == 3) {
            cmd.type = CMD_UPDATE;
            cmd.query_id = id;
            strncpy(cmd.row.name, name, sizeof(cmd.row.name));
            cmd.row.age = age;
        }
    } else if (strncmp(input, "delete", 6) == 0) {
        int id;
        if (sscanf(input, "delete %d", &id) == 1) {
            cmd.type = CMD_DELETE;
            cmd.query_id = id;
        }
    } else if (strncmp(input, "exit", 4) == 0) {
        cmd.type = CMD_EXIT;
    }

    return cmd;
}
