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
    } else if (strncmp(input, "select where id=", 16) == 0) {
        int id;
        if (sscanf(input, "select where id=%d", &id) == 1) {
            cmd.type = CMD_SELECT_ID;
            cmd.query_id = id;
        }
    } else if (strncmp(input, "select", 6) == 0) {
        cmd.type = CMD_SELECT_ALL;
    } else if (strncmp(input, "exit", 4) == 0) {
        cmd.type = CMD_EXIT;
    }

    return cmd;
}