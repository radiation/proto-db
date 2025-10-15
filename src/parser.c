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

static void to_lowercase(char* s) {
    for (size_t i = 0; s[i]; i++) {
        // Only convert A–Z (leave quotes and literals intact)
        if (s[i] >= 'A' && s[i] <= 'Z') {
            s[i] = (char)(s[i] + 32);
        }
    }
}

// Remove trailing whitespace, semicolons, and normalize spacing
static void sanitize_input(char* s) {
    // 1️⃣ Trim trailing whitespace, newlines, and semicolons
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ';' || s[len - 1] == '\n' || s[len - 1] == ' ')) {
        s[--len] = '\0';
    }

    // 2️⃣ Trim leading whitespace
    size_t start = 0;
    while (s[start] == ' ' || s[start] == '\t')
        start++;
    if (start > 0)
        memmove(s, s + start, len - start + 1);

    // 3️⃣ Normalize internal whitespace (collapse multiple spaces)
    char* dst = s;
    int space_seen = 0;
    for (char* src = s; *src; src++) {
        if (*src == ' ' || *src == '\t') {
            if (!space_seen) *dst++ = ' ';
            space_seen = 1;
        } else {
            *dst++ = *src;
            space_seen = 0;
        }
    }
    *dst = '\0';

    // 4️⃣ Trim dangling quotes
    if (s[0] == '"' && s[strlen(s) - 1] == '"') {
        memmove(s, s + 1, strlen(s) - 2);
        s[strlen(s) - 2] = '\0';
    } else if (s[0] == '\'' && s[strlen(s) - 1] == '\'') {
        memmove(s, s + 1, strlen(s) - 2);
        s[strlen(s) - 2] = '\0';
    }
}

Command parse_command(const char* input) {
    Command cmd;
    cmd.type = CMD_UNKNOWN;

    // Copy input to a mutable buffer
    char buffer[256];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Normalize input
    sanitize_input(buffer);
    to_lowercase(buffer);
    input = buffer;
    
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

    if (strncmp(input, "describe", 8) == 0) {
        cmd.type = CMD_DESCRIBE;
        char table_name[32];

        if (sscanf(input, "describe %31s", table_name) == 1) {
            strncpy(cmd.describe.table_name, table_name, sizeof(cmd.describe.table_name));
        } else {
            cmd.describe.table_name[0] = '\0';
        }

        return cmd;
    }

    // INSERT INTO <table> <val1> <val2> ...
    if (strncmp(input, "insert into", 11) == 0) {
        cmd.type = CMD_INSERT;

        char table_name[32];
        const char* p = input + 11;
        while (*p == ' ') p++;

        // Read table name
        if (sscanf(p, "%31s", table_name) != 1)
            return cmd;

        strncpy(cmd.insert.table_name, table_name, sizeof(cmd.insert.table_name));

        // Move past table name
        p = strstr(p, table_name);
        p += strlen(table_name);

        // Check for optional column list
        char cols_part[128] = {0};
        char vals_part[128] = {0};

        const char* open_paren = strchr(p, '(');
        const char* close_paren = strchr(p, ')');
        const char* values_kw = strstr(p, "values");

        if (open_paren && close_paren && values_kw) {
            // Extract column names between parentheses
            strncpy(cols_part, open_paren + 1, close_paren - open_paren - 1);
            cols_part[close_paren - open_paren - 1] = '\0';

            // Extract values part
            const char* val_open = strchr(values_kw, '(');
            const char* val_close = strchr(values_kw, ')');
            if (val_open && val_close && val_close > val_open) {
                strncpy(vals_part, val_open + 1, val_close - val_open - 1);
                vals_part[val_close - val_open - 1] = '\0';
            }

            // Tokenize both lists
            char* col_tok = strtok(cols_part, ",");
            char* val_tok = strtok(vals_part, ",");

            cmd.insert.value_count = 0;

            while (col_tok && val_tok && cmd.insert.value_count < MAX_COLUMNS) {
                ColumnValue* val = &cmd.insert.values[cmd.insert.value_count++];

                // Clean whitespace
                while (*col_tok == ' ') col_tok++;
                while (*val_tok == ' ') val_tok++;

                strncpy(val->column, col_tok, sizeof(val->column));

                // Detect type
                if (val_tok[0] == '"' || val_tok[0] == '\'') {
                    val->type = VALUE_STRING;
                    strncpy(val->str_val, val_tok + 1,
                            strlen(val_tok) - 2); // remove quotes
                    val->str_val[strlen(val_tok) - 2] = '\0';
                } else {
                    val->type = VALUE_INT;
                    val->int_val = atoi(val_tok);
                }

                col_tok = strtok(NULL, ",");
                val_tok = strtok(NULL, ",");
            }

            return cmd;
        }

        // Legacy fallback: insert into users Alice 30
        char val1[64], val2[64], val3[64];
        int n = sscanf(input, "insert into %31s %63s %63s %63s",
                    table_name, val1, val2, val3);
        if (n >= 3) {
            strncpy(cmd.insert.table_name, table_name,
                    sizeof(cmd.insert.table_name));
            cmd.insert.value_count = n - 1;
            if (is_number(val1)) {
                cmd.insert.values[0].type = VALUE_INT;
                cmd.insert.values[0].int_val = atoi(val1);
            } else {
                cmd.insert.values[0].type = VALUE_STRING;
                strncpy(cmd.insert.values[0].str_val, val1,
                        sizeof(cmd.insert.values[0].str_val));
            }
            // etc. (leave your old fallback handling here)
        }

        return cmd;
    }

    if (strncmp(input, "select from", 11) == 0 && strstr(input, "where")) {
        cmd.type = CMD_SELECT_COND;

        char table_name[32], field[32], op[3], value[64];
        int parsed = sscanf(input, "select from %31s where %31[^<>=! ] %2[<>=!] %63[^\n]",
                            table_name, field, op, value);
        if (parsed != 4) return cmd;

        strncpy(cmd.select_where.table_name, table_name, sizeof(cmd.select_where.table_name));
        cmd.select_where.conds.cond_count = 1;
        cmd.select_where.conds.op_count = 0;

        Condition* c = &cmd.select_where.conds.conds[0];
        strncpy(c->field, field, sizeof(c->field));

        if (strcmp(op, "=") == 0) c->op = OP_EQ;
        else if (strcmp(op, "!=") == 0) c->op = OP_NEQ;
        else if (strcmp(op, ">") == 0) c->op = OP_GT;
        else if (strcmp(op, "<") == 0) c->op = OP_LT;
        else if (strcmp(op, ">=") == 0) c->op = OP_GTE;
        else if (strcmp(op, "<=") == 0) c->op = OP_LTE;
        else return cmd;

        if (value[0] == '"' || value[0] == '\'') {
            c->int_value = 0;
            strncpy(c->str_value, value + 1, strlen(value) - 2);
            c->str_value[strlen(value) - 2] = '\0';
        } else {
            c->int_value = atoi(value);
            c->str_value[0] = '\0';
        }

        return cmd;
    }

    if (strncmp(input, "select from", 11) == 0) {
        cmd.type = CMD_SELECT_ALL;

        char table_name[32];
        if (sscanf(input, "select from %31s", table_name) != 1)
            return cmd;

        strncpy(cmd.select_all.table_name, table_name, sizeof(cmd.select_all.table_name));
        return cmd;
    }

    // UPDATE ... WHERE ... SET ...
    if (strncmp(input, "update", 6) == 0 && strstr(input, "where") && strstr(input, "set")) {
        cmd.type = CMD_UPDATE_WHERE;

        char table_name[32], where_field[32], where_op[3], where_val[64];
        char set_field[32], set_val[64];

        int matched = sscanf(input,
            "update %31s set %31[^= ] = %63[^ ] where %31[^<>=! ] %2[<>=!] %63[^\n]",
            table_name, set_field, set_val, where_field, where_op, where_val);

        if (matched != 6) {
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
