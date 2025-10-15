#include "query.h"
#include "catalog.h"
#include <stdio.h>
#include <string.h>

int eval_condition(ColumnValue* row, int column_count, const Condition* cond) {
    for (int i = 0; i < column_count; i++) {
        if (strcmp(row[i].column, cond->field) != 0)
            continue;

        if (row[i].type == VALUE_INT && cond->str_value[0] == '\0') {
            // Integer comparison
            switch (cond->op) {
                case OP_EQ:  return row[i].int_val == cond->int_value;
                case OP_GT:  return row[i].int_val >  cond->int_value;
                case OP_LT:  return row[i].int_val <  cond->int_value;
                case OP_NEQ: return row[i].int_val != cond->int_value;
                case OP_GTE: return row[i].int_val >= cond->int_value;
                case OP_LTE: return row[i].int_val <= cond->int_value;
                default:     return 0;
            }
        }
        else if (row[i].type == VALUE_STRING) {
            // Ttrim whitespace and quotes
            const char* val = row[i].str_val;
            const char* cond_str = cond->str_value;
            char lhs[64], rhs[64];
            strncpy(lhs, val, sizeof(lhs));
            strncpy(rhs, cond_str, sizeof(rhs));
            lhs[sizeof(lhs)-1] = '\0';
            rhs[sizeof(rhs)-1] = '\0';

            // Trim spaces and quotes (e.g. "charlie" → charlie)
            char* start = rhs;
            while (*start == '"' || *start == '\'' || *start == ' ') start++;
            char* end = rhs + strlen(rhs) - 1;
            while (end > start && (*end == '"' || *end == '\'' || *end == ' ')) *end-- = '\0';

            int cmp = strcmp(lhs, start);
            switch (cond->op) {
                case OP_EQ:  return cmp == 0;
                case OP_NEQ: return cmp != 0;
                default:     return 0; // no GT/LT for strings yet
            }
        }
    }

    return 0;
}

int eval_condition_list(ColumnValue* row, int column_count, const ConditionList* conds) {
    if (conds->cond_count == 0) return 1;

    int result = eval_condition(row, column_count, &conds->conds[0]);
    for (int i = 0; i < conds->op_count; i++) {
        int rhs = eval_condition(row, column_count, &conds->conds[i + 1]);
        if (conds->ops[i] == LOGICAL_AND) result = result && rhs;
        else if (conds->ops[i] == LOGICAL_OR) result = result || rhs;
    }
    return result;
}

void scan_rows(const char* table_name, const TableDef* def, const ConditionList* conds, RowCallback callback) {
    char path[256];
    snprintf(path, sizeof(path), "data/%s.db", table_name);
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        return;
    }

    TableHeader header;
    fread(&header, sizeof(TableHeader), 1, f);

    ColumnValue row[MAX_COLUMNS];

    for (int i = 0; i < header.num_rows; i++) {
        long offset = sizeof(TableHeader) + i * 1024;  // assumes fixed row size
        fseek(f, offset, SEEK_SET);

        for (int j = 0; j < def->num_columns; j++) {
            ColumnDef col = def->columns[j];
            ColumnValue* val = &row[j];

            strncpy(val->column, col.name, sizeof(val->column));
            val->type = (col.type == COL_INT) ? VALUE_INT : VALUE_STRING;

            if (col.type == COL_INT) {
                fread(&val->int_val, sizeof(int), 1, f);
            } else {
                fread(val->str_val, col.length, 1, f);
                val->str_val[col.length - 1] = '\0';  // safety
            }
        }

        if (eval_condition_list(row, def->num_columns, conds)) {
            callback(row, def->num_columns, f, offset);
        }
    }

    fclose(f);
}