#ifndef DB_RUNTIME_H
#define DB_RUNTIME_H

void db_connect(const char *connstr);
void db_disconnect(void);

void db_ensure_table_int(const char *name);
void db_set_int(const char *name, int val);
int  db_get_int(const char *name);

void  db_ensure_table_float(const char *name);
void  db_set_float(const char *name, float val);
float db_get_float(const char *name);

void   db_ensure_table_double(const char *name);
void   db_set_double(const char *name, double val);
double db_get_double(const char *name);

void db_ensure_table_char(const char *name);
void db_set_char(const char *name, char val);
char db_get_char(const char *name);

void  db_ensure_table_str(const char *name);
void  db_set_str(const char *name, const char *val);
char *db_get_str(const char *name);

#endif
