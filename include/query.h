#ifndef QUERY_H
#define QUERY_H

#include "db.h"
#include "command.h"
#include <stdio.h>

int eval_condition(ColumnValue* row, int column_count, const Condition* cond);
int eval_condition_list(ColumnValue* row, int column_count, const ConditionList* conds);

typedef void (*RowCallback)(ColumnValue* row, int column_count, FILE* f, long offset);

void scan_rows(const char* table_name, const TableDef* def, const ConditionList* conds, RowCallback callback);

#endif
