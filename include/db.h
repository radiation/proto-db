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
} Row;

typedef enum {
    CMD_INSERT,
    CMD_SELECT_ALL,
    CMD_SELECT_ID,
    CMD_EXIT,
    CMD_UNKNOWN
} CommandType;

typedef struct {
    CommandType type;
    Row row;
    int query_id;
} Command;

void db_insert(Row* row);
void db_select_all();
void db_select_by_id(int id);

#endif
