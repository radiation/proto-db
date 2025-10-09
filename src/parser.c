#include "command.h"
#include "parser.h"
#include "where_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int is_number(const char* s) {
    if (!s || *s == '\0') return 0;
    if (*s == '-' || *s == '+') s++;
    while (*s) {
        if (!isdigit(*s)) return 0;
        s++;
    }
    return 1;
}

Command parse_command(const char* input) {
    Command cmd;
    cmd.type = CMD_UNKNOWN;

    // CREATE TABLE
    if (strncmp(input, "create table", 12) == 0) {
        cmd.type = CMD_CREATE_TABLE;

        char table_name[32];
        if (sscanf(input, "create table %31s", table_name) != 1) return cmd;
        strncpy(cmd.create_table.table_name, table_name, sizeof(cmd.create_table.table_name));

        const char* open = strchr(input, '(');
        const char* close = strchr(input, ')');
        if (!open || !close || close <= open) return cmd;

        char columns_part[256];
        strncpy(columns_part, open + 1, close - open - 1);
        columns_part[close - open - 1] = '\0';

        cmd.create_table.column_count = 0;
        char* token = strtok(columns_part, ",");
        while (token && cmd.create_table.column_count < MAX_COLUMNS) {
            char col_name[32], type_str[16];
            if (sscanf(token, " %31s %15s", col_name, type_str) != 2) break;

            ColumnDef* col = &cmd.create_table.columns[cmd.create_table.column_count++];
            strncpy(col->name, col_name, sizeof(col->name));

            if (strcmp(type_str, "int") == 0) {
                col->type = COL_INT;
                col->length = 0;
            } else if (strcmp(type_str, "string") == 0) {
                col->type = COL_STRING;
                col->length = 32;
            } else {
                break;
            }

            token = strtok(NULL, ",");
        }

        return cmd;
    }

    // INSERT INTO <table> <val1> <val2> ...
    if (strncmp(input, "insert into", 11) == 0) {
        cmd.type = CMD_INSERT;
        char table_name[32], val1[64], val2[64];
        if (sscanf(input, "insert into %31s %63s %63s", table_name, val1, val2) != 3)
            return cmd;

        strncpy(cmd.insert.table_name, table_name, sizeof(cmd.insert.table_name));

        // Value 1
        strncpy(cmd.insert.values[0].column, "col1", sizeof(cmd.insert.values[0].column));
        if (is_number(val1)) {
            cmd.insert.values[0].type = VALUE_INT;
            cmd.insert.values[0].int_val = atoi(val1);
        } else {
            cmd.insert.values[0].type = VALUE_STRING;
            strncpy(cmd.insert.values[0].str_val, val1, sizeof(cmd.insert.values[0].str_val));
        }

        // Value 2
        strncpy(cmd.insert.values[1].column, "col2", sizeof(cmd.insert.values[1].column));
        if (is_number(val2)) {
            cmd.insert.values[1].type = VALUE_INT;
            cmd.insert.values[1].int_val = atoi(val2);
        } else {
            cmd.insert.values[1].type = VALUE_STRING;
            strncpy(cmd.insert.values[1].str_val, val2, sizeof(cmd.insert.values[1].str_val));
        }

        cmd.insert.value_count = 2;
        return cmd;
    }

    // SELECT ALL (for now)
    if (strncmp(input, "select from", 11) == 0) {
        cmd.type = CMD_SELECT_ALL;
        sscanf(input, "select from %31s", cmd.select_all.table_name);
        return cmd;
    }

    // UPDATE ... WHERE ... SET ...
    if (strncmp(input, "update", 6) == 0 && strstr(input, "where") && strstr(input, "set")) {
        cmd.type = CMD_UPDATE_WHERE;

        char table_name[32], where_field[32], where_op[3], where_val[64];
        char set_field[32], set_val[64];

        if (sscanf(input, "update %31s where %31s %2s %63s set %31s = %63s",
                   table_name, where_field, where_op, where_val, set_field, set_val) != 6) {
            return cmd;
        }

        strncpy(cmd.update_where.table_name, table_name, sizeof(cmd.update_where.table_name));
        cmd.update_where.conds.cond_count = 1;
        cmd.update_where.conds.op_count = 0;

        // WHERE
        Condition* c = &cmd.update_where.conds.conds[0];
        strncpy(c->field, where_field, sizeof(c->field));
        if (strcmp(where_op, "=") == 0) c->op = OP_EQ;
        else if (strcmp(where_op, ">") == 0) c->op = OP_GT;
        else if (strcmp(where_op, "<") == 0) c->op = OP_LT;
        else if (strcmp(where_op, "!=") == 0) c->op = OP_NEQ;
        else return cmd;

        if (is_number(where_val)) {
            c->int_value = atoi(where_val);
            c->str_value[0] = '\0';
        } else {
            strncpy(c->str_value, where_val, sizeof(c->str_value));
            c->int_value = 0;
        }

        // SET
        cmd.update_where.update_count = 1;
        ColumnValue* col = &cmd.update_where.updates[0];
        strncpy(col->column, set_field, sizeof(col->column));

        if (is_number(set_val)) {
            col->type = VALUE_INT;
            col->int_val = atoi(set_val);
        } else {
            col->type = VALUE_STRING;
            strncpy(col->str_val, set_val, sizeof(col->str_val));
        }

        return cmd;
    }

    // DELETE WHERE
    if (strncmp(input, "delete from", 11) == 0 && strstr(input, "where")) {
        cmd.type = CMD_DELETE_WHERE;

        char table_name[32], field[32], op[3], val[64];
        if (sscanf(input, "delete from %31s where %31s %2s %63s", table_name, field, op, val) != 4)
            return cmd;

        strncpy(cmd.delete_where.table_name, table_name, sizeof(cmd.delete_where.table_name));
        cmd.delete_where.conds.cond_count = 1;
        cmd.delete_where.conds.op_count = 0;

        Condition* c = &cmd.delete_where.conds.conds[0];
        strncpy(c->field, field, sizeof(c->field));
        if (strcmp(op, "=") == 0) c->op = OP_EQ;
        else if (strcmp(op, ">") == 0) c->op = OP_GT;
        else if (strcmp(op, "<") == 0) c->op = OP_LT;
        else return cmd;

        if (is_number(val)) {
            c->int_value = atoi(val);
            c->str_value[0] = '\0';
        } else {
            strncpy(c->str_value, val, sizeof(c->str_value));
            c->int_value = 0;
        }

        return cmd;
    }

    // EXIT
    if (strncmp(input, "exit", 4) == 0) {
        cmd.type = CMD_EXIT;
        return cmd;
    }

    return cmd;
}
