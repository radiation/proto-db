#include "where_parser.h"
#include <string.h>
#include <stdlib.h>

ConditionList parse_where_clause(char* input) {
    ConditionList conds;
    conds.conds = malloc(sizeof(Condition) * 16);
    conds.cond_count = 0;
    conds.ops = malloc(sizeof(LogicalOp) * 16);
    conds.op_count = 0;

    char* token = strtok(input, " ");
    while (token) {
        Condition c;
        char* field = token;
        char* op = strtok(NULL, " ");
        char* value = strtok(NULL, " ");

        if (!field || !op || !value) break;

        if (strcmp(field, "id") == 0) c.field = FIELD_ID;
        else if (strcmp(field, "age") == 0) c.field = FIELD_AGE;
        else if (strcmp(field, "name") == 0) c.field = FIELD_NAME;

        if (strcmp(op, "=") == 0) c.op = OP_EQ;
        else if (strcmp(op, ">") == 0) c.op = OP_GT;
        else if (strcmp(op, "<") == 0) c.op = OP_LT;

        if (c.field == FIELD_NAME) strncpy(c.str_value, value, sizeof(c.str_value));
        else c.int_value = atoi(value);

        conds.conds[conds.cond_count++] = c;

        char* next = strtok(NULL, " ");
        if (next && (strcmp(next, "and") == 0 || strcmp(next, "or") == 0)) {
            conds.ops[conds.op_count++] =
                (strcmp(next, "and") == 0) ? LOGICAL_AND : LOGICAL_OR;
            token = strtok(NULL, " ");
        } else {
            break;
        }
    }

    return conds;
}
