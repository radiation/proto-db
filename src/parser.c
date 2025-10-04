#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
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
    } else if (strncmp(input, "select where", 12) == 0) {
        Command cmd;
        cmd.type = CMD_SELECT_COND;

        cmd.conds.conds = malloc(sizeof(Condition) * 16); // start small
        cmd.conds.cond_count = 0;
        cmd.conds.ops = malloc(sizeof(LogicalOp) * 16);
        cmd.conds.op_count = 0;

        char* token = strtok((char*)input, " "); // destructive split
        // skip "select" and "where"
        token = strtok(NULL, " "); // should be "where"
        token = strtok(NULL, " "); // first field

        while (token) {
            Condition c;
            char* field = token;
            char* op = strtok(NULL, " ");
            char* value = strtok(NULL, " ");

            // fill condition
            if (strcmp(field, "id") == 0) c.field = FIELD_ID;
            else if (strcmp(field, "age") == 0) c.field = FIELD_AGE;
            else if (strcmp(field, "name") == 0) c.field = FIELD_NAME;

            if (strcmp(op, "=") == 0) c.op = OP_EQ;
            else if (strcmp(op, ">") == 0) c.op = OP_GT;
            else if (strcmp(op, "<") == 0) c.op = OP_LT;

            if (c.field == FIELD_NAME) strncpy(c.str_value, value, sizeof(c.str_value));
            else c.int_value = atoi(value);

            // add condition to list
            cmd.conds.conds[cmd.conds.cond_count++] = c;

            // check for logical operator
            char* next = strtok(NULL, " ");
            if (next && (strcmp(next, "and") == 0 || strcmp(next, "or") == 0)) {
                if (strcmp(next, "and") == 0) cmd.conds.ops[cmd.conds.op_count++] = LOGICAL_AND;
                else cmd.conds.ops[cmd.conds.op_count++] = LOGICAL_OR;
                token = strtok(NULL, " "); // move to next field
            } else {
                break; // no more conditions
            }
        }

        return cmd;
    } else if (strncmp(input, "select", 6) == 0) {
        cmd.type = CMD_SELECT_ALL;
    } else if (strncmp(input, "update where", 12) == 0) {
        Command cmd;
        cmd.type = CMD_UPDATE_WHERE;

        // Parse the WHERE part
        cmd.conds.conds = malloc(sizeof(Condition) * 16);
        cmd.conds.cond_count = 0;
        cmd.conds.ops = malloc(sizeof(LogicalOp) * 16);
        cmd.conds.op_count = 0;

        char* token = strtok((char*)input, " ");
        token = strtok(NULL, " "); // "where"
        token = strtok(NULL, " "); // first field

        while (token) {
            Condition c;
            char* field = token;
            char* op = strtok(NULL, " ");
            char* value = strtok(NULL, " ");

            if (strcmp(field, "id") == 0) c.field = FIELD_ID;
            else if (strcmp(field, "age") == 0) c.field = FIELD_AGE;
            else if (strcmp(field, "name") == 0) c.field = FIELD_NAME;

            if (strcmp(op, "=") == 0) c.op = OP_EQ;
            else if (strcmp(op, ">") == 0) c.op = OP_GT;
            else if (strcmp(op, "<") == 0) c.op = OP_LT;

            if (c.field == FIELD_NAME) strncpy(c.str_value, value, sizeof(c.str_value));
            else c.int_value = atoi(value);

            cmd.conds.conds[cmd.conds.cond_count++] = c;

            char* next = strtok(NULL, " ");
            if (next && (strcmp(next, "and") == 0 || strcmp(next, "or") == 0)) {
                if (strcmp(next, "and") == 0) cmd.conds.ops[cmd.conds.op_count++] = LOGICAL_AND;
                else cmd.conds.ops[cmd.conds.op_count++] = LOGICAL_OR;
                token = strtok(NULL, " ");
            } else if (next && strcmp(next, "set") == 0) {
                // reached the SET clause
                break;
            } else {
                break;
            }
        }

        // Parse the SET clause
        char* field = strtok(NULL, " "); // field to set
        char* eq = strtok(NULL, " ");    // '='
        char* value = strtok(NULL, " "); // new value

        cmd.row.id = -1; // unused
        if (strcmp(field, "age") == 0) {
            cmd.row.age = atoi(value);
            cmd.row.name[0] = '\0';
        } else if (strcmp(field, "name") == 0) {
            strncpy(cmd.row.name, value, sizeof(cmd.row.name));
            cmd.row.age = -1;
        }

        return cmd;
    } else if (strncmp(input, "update", 6) == 0) {
        int id, age;
        char name[32];
        if (sscanf(input, "update %d %31s %d", &id, name, &age) == 3) {
            cmd.type = CMD_UPDATE;
            cmd.query_id = id;
            strncpy(cmd.row.name, name, sizeof(cmd.row.name));
            cmd.row.age = age;
        }
    } else if (strncmp(input, "delete where", 12) == 0) {
        Command cmd;
        cmd.type = CMD_DELETE_WHERE;

        // reuse the same multi-condition parser logic from "select where"
        cmd.conds.conds = malloc(sizeof(Condition) * 16);
        cmd.conds.cond_count = 0;
        cmd.conds.ops = malloc(sizeof(LogicalOp) * 16);
        cmd.conds.op_count = 0;

        char* token = strtok((char*)input, " ");
        token = strtok(NULL, " "); // "where"
        token = strtok(NULL, " "); // first field

        while (token) {
            Condition c;
            char* field = token;
            char* op = strtok(NULL, " ");
            char* value = strtok(NULL, " ");

            if (strcmp(field, "id") == 0) c.field = FIELD_ID;
            else if (strcmp(field, "age") == 0) c.field = FIELD_AGE;
            else if (strcmp(field, "name") == 0) c.field = FIELD_NAME;

            if (strcmp(op, "=") == 0) c.op = OP_EQ;
            else if (strcmp(op, ">") == 0) c.op = OP_GT;
            else if (strcmp(op, "<") == 0) c.op = OP_LT;

            if (c.field == FIELD_NAME) strncpy(c.str_value, value, sizeof(c.str_value));
            else c.int_value = atoi(value);

            cmd.conds.conds[cmd.conds.cond_count++] = c;

            char* next = strtok(NULL, " ");
            if (next && (strcmp(next, "and") == 0 || strcmp(next, "or") == 0)) {
                if (strcmp(next, "and") == 0) cmd.conds.ops[cmd.conds.op_count++] = LOGICAL_AND;
                else cmd.conds.ops[cmd.conds.op_count++] = LOGICAL_OR;
                token = strtok(NULL, " ");
            } else {
                break;
            }
        }

        return cmd;
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
