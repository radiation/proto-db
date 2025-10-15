#pragma once
#include "catalog.h"  // for ColumnDef
#include "db.h"       // for ConditionList

#define DB_OK     0
#define DB_EXIT   -1
#define DB_ERROR  -2

typedef enum {
    CMD_CREATE_TABLE,
    CMD_DESCRIBE,
    CMD_INSERT,
    CMD_SELECT_ALL,
    CMD_SELECT_COND,
    CMD_UPDATE_WHERE,
    CMD_DELETE_WHERE,
    CMD_EXIT,
    CMD_UNKNOWN
} CommandType;

typedef enum {
    VALUE_INT,
    VALUE_STRING
} ValueType;

typedef struct {
    char column[32];
    ValueType type;
    union {
        int int_val;
        char str_val[64];
    };
} ColumnValue;

typedef struct {
    char table_name[32];
    ColumnDef columns[16];
    int column_count;
} CreateTableCommand;

typedef struct {
    char table_name[32];
} DescribeCommand;

typedef struct {
    char table_name[32];
    ColumnValue values[16];
    int value_count;
} InsertCommand;

typedef struct {
    char table_name[32];
    ConditionList conds;
} SelectWhereCommand;

typedef struct {
    char table_name[32];
    ConditionList conds;
} DeleteWhereCommand;

typedef struct {
    char table_name[32];
    ConditionList conds;
    ColumnValue updates[16];
    int update_count;
} UpdateWhereCommand;

typedef struct {
    char table_name[32];
} SelectAllCommand;

typedef struct {
    CommandType type;
    union {
        CreateTableCommand create_table;
        DescribeCommand describe;
        InsertCommand insert;
        SelectWhereCommand select_where;
        SelectAllCommand select_all;
        UpdateWhereCommand update_where;
        DeleteWhereCommand delete_where;
    };
} Command;

int db_execute_command(const Command* cmd);
