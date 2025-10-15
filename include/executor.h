#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "db.h"
#include "command.h"

int db_execute_command(const Command* cmd);

int db_insert_row(const char* table_name, ColumnValue* values, int count);
int db_select_all(const char* table_name);
int db_select_where(const char* table_name, const ConditionList* conds);
int db_update_where(const char* table_name, const ConditionList* conds, const ColumnValue* updates, int update_count);
int db_delete_where(const char* table_name, const ConditionList* conds);

#endif
