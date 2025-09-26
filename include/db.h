#ifndef DB_H
#define DB_H

#define NAME_SIZE 32

typedef struct {
    int id;
    char name[NAME_SIZE];
    int age;
} Row;

void db_insert(const Row* row);
void db_select_all();

#endif
