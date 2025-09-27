#ifndef DB_H
#define DB_H

#define NAME_SIZE 32

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
    OP_LT    // <
} Operator;

typedef enum {
    CMD_INSERT,
    CMD_SELECT_ALL,
    CMD_SELECT_COND,
    CMD_UPDATE,
    CMD_DELETE,
    CMD_EXIT,
    CMD_UNKNOWN
} CommandType;

typedef struct {
    CommandType type;
    Field field;        // FIELD_ID, FIELD_NAME, FIELD_AGE
    Operator op;        // =, >, <
    char str_value[32]; // for string compares
    int int_value;      // for numeric compares
    Row row;            // still used for insert/update
    int query_id;       // kept for backwards compatibility
} Command;

void db_init();
void db_insert(Row* row);
void db_select_all();
void db_select_where(Field field, Operator op, const char* str_val, int int_val);
void db_update_by_id(int id, const char* new_name, int new_age);
void db_delete_by_id(int id);

#endif
