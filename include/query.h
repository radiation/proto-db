#ifndef QUERY_H
#define QUERY_H

#include "db.h"
#include <stdio.h>

int eval_condition(Row* r, Condition* cond);
int eval_condition_list(Row* r, ConditionList* conds);

typedef void (*RowCallback)(Row* r, long offset, FILE* f);

void scan_rows(ConditionList* conds, RowCallback callback);

#endif
