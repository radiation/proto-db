#ifndef DB_H
#define DB_H

#define NAME_SIZE 32
#define MAX_CONDITIONS 4

typedef struct {
    int num_rows;
    int next_id;
} DbHeader;

typedef struct {
    int id;
    char name[NAME_SIZE];
    int age;
    int is_deleted;
} Row;

typedef enum {
    FIELD_ID,
    FIELD_NAME,
    FIELD_AGE
} Field;

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

typedef struct {
    Field field;
    Operator op;
    char str_value[32];
    int int_value;
} Condition;

typedef struct {
    Condition* conds;
    int cond_count;
    LogicalOp* ops;
    int op_count;
} ConditionList;

typedef enum {
    CMD_INSERT,
    CMD_SELECT_ALL,
    CMD_SELECT_COND,
    CMD_UPDATE,
    CMD_UPDATE_WHERE,
    CMD_DELETE,
    CMD_DELETE_WHERE,
    CMD_EXIT,
    CMD_UNKNOWN
} CommandType;

typedef struct {
    CommandType type;
    ConditionList conds;
    Row row;
    int query_id;
} Command;

void db_init();
void db_insert(Row* row);
void db_select_all();
void db_select_where(Field field, Operator op, const char* str_val, int int_val);
void db_select_where_list(ConditionList* conds);
void db_update_by_id(int id, const char* new_name, int new_age);
void db_update_where(ConditionList* conds, const Row* new_values);
void db_delete_by_id(int id);
void db_delete_where(ConditionList* conds);

#endif
