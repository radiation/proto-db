#include "query.h"
#include "db.h"
#include <stdio.h>
#include <string.h>

int eval_condition(Row* r, Condition* cond) {
    switch (cond->field) {
        case FIELD_ID:
            if (cond->op == OP_EQ) return r->id == cond->int_value;
            if (cond->op == OP_GT) return r->id > cond->int_value;
            if (cond->op == OP_LT) return r->id < cond->int_value;
            break;
        case FIELD_AGE:
            if (cond->op == OP_EQ) return r->age == cond->int_value;
            if (cond->op == OP_GT) return r->age > cond->int_value;
            if (cond->op == OP_LT) return r->age < cond->int_value;
            break;
        case FIELD_NAME:
            if (cond->op == OP_EQ) return strcmp(r->name, cond->str_value) == 0;
            break;
    }
    return 0;
}

int eval_condition_list(Row* r, ConditionList* conds) {
    if (conds->cond_count == 0) return 1;

    int result = eval_condition(r, &conds->conds[0]);
    for (int j = 0; j < conds->op_count; j++) {
        int rhs = eval_condition(r, &conds->conds[j + 1]);
        if (conds->ops[j] == LOGICAL_AND) result = result && rhs;
        else if (conds->ops[j] == LOGICAL_OR) result = result || rhs;
    }
    return result;
}

void scan_rows(ConditionList* conds, RowCallback callback) {
    FILE* f = fopen(DB_FILE, "r+b");
    if (!f) { perror("fopen"); return; }

    load_header(f);
    DbHeader* header = get_header();
    Row r;

    for (int i = 0; i < header->num_rows; i++) {
        long offset = sizeof(DbHeader) + i * sizeof(Row);
        fseek(f, offset, SEEK_SET);
        fread(&r, sizeof(Row), 1, f);
        if (r.is_deleted) continue;

        if (eval_condition_list(&r, conds)) {
            callback(&r, offset, f);
        }
    }

    fclose(f);
}