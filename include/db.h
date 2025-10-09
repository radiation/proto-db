#ifndef DB_H
#define DB_H

#include <stdio.h>

#define MAX_CONDITIONS 4

// --- Core logical/conditional operators ---
typedef enum {
    OP_EQ,   // =
    OP_GT,   // >
    OP_LT,   // <
    OP_NEQ,  // !=
    OP_GTE,  // >=
    OP_LTE   // <=
} Operator;

typedef enum {
    LOGICAL_NONE,
    LOGICAL_AND,
    LOGICAL_OR
} LogicalOp;

// --- WHERE clause condition ---
typedef struct {
    char field[32];
    Operator op;
    char str_value[32];
    int int_value;
} Condition;

typedef struct {
    Condition conds[MAX_CONDITIONS];
    int cond_count;
    LogicalOp ops[MAX_CONDITIONS - 1];
    int op_count;
} ConditionList;

// --- Table header for on-disk files ---
typedef struct {
    int num_rows;
    int next_id;
} DbHeader;

DbHeader* get_header(void);
void load_header(FILE* f);
void db_init();

#endif
