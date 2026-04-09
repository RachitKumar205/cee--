#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "db_runtime.h"

static PGconn *conn = NULL;

static void check_conn(void) {
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "DB connection lost: %s\n", PQerrorMessage(conn));
        exit(1);
    }
}

static void exec_sql(const char *sql) {
    check_conn();
    PGresult *res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_COMMAND_OK &&
        PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SQL error: %s\nSQL: %s\n", PQerrorMessage(conn), sql);
        PQclear(res);
        exit(1);
    }
    PQclear(res);
}

void db_connect(const char *connstr) {
    conn = PQconnectdb(connstr);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }
}

void db_disconnect(void) {
    if (conn) {
        PQfinish(conn);
        conn = NULL;
    }
}

/* ── int ── */

void db_ensure_table_int(const char *name) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "CREATE TABLE IF NOT EXISTS var_%s "
        "(id INTEGER PRIMARY KEY, value INTEGER NOT NULL);",
        name);
    exec_sql(sql);
}

void db_set_int(const char *name, int val) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO var_%s (id, value) VALUES (1, %d) "
        "ON CONFLICT (id) DO UPDATE SET value = EXCLUDED.value;",
        name, val);
    exec_sql(sql);
}

int db_get_int(const char *name) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT value FROM var_%s WHERE id = 1;", name);
    check_conn();
    PGresult *res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        fprintf(stderr, "db_get_int: no value for var_%s\n", name);
        PQclear(res);
        exit(1);
    }
    int val = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return val;
}

/* ── float ── */

void db_ensure_table_float(const char *name) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "CREATE TABLE IF NOT EXISTS var_%s "
        "(id INTEGER PRIMARY KEY, value REAL NOT NULL);",
        name);
    exec_sql(sql);
}

void db_set_float(const char *name, float val) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO var_%s (id, value) VALUES (1, %f) "
        "ON CONFLICT (id) DO UPDATE SET value = EXCLUDED.value;",
        name, val);
    exec_sql(sql);
}

float db_get_float(const char *name) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT value FROM var_%s WHERE id = 1;", name);
    check_conn();
    PGresult *res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        fprintf(stderr, "db_get_float: no value for var_%s\n", name);
        PQclear(res);
        exit(1);
    }
    float val = (float)atof(PQgetvalue(res, 0, 0));
    PQclear(res);
    return val;
}

/* ── double ── */

void db_ensure_table_double(const char *name) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "CREATE TABLE IF NOT EXISTS var_%s "
        "(id INTEGER PRIMARY KEY, value DOUBLE PRECISION NOT NULL);",
        name);
    exec_sql(sql);
}

void db_set_double(const char *name, double val) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO var_%s (id, value) VALUES (1, %f) "
        "ON CONFLICT (id) DO UPDATE SET value = EXCLUDED.value;",
        name, val);
    exec_sql(sql);
}

double db_get_double(const char *name) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT value FROM var_%s WHERE id = 1;", name);
    check_conn();
    PGresult *res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        fprintf(stderr, "db_get_double: no value for var_%s\n", name);
        PQclear(res);
        exit(1);
    }
    double val = atof(PQgetvalue(res, 0, 0));
    PQclear(res);
    return val;
}

/* ── char ── */

void db_ensure_table_char(const char *name) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "CREATE TABLE IF NOT EXISTS var_%s "
        "(id INTEGER PRIMARY KEY, value CHAR(1) NOT NULL);",
        name);
    exec_sql(sql);
}

void db_set_char(const char *name, char val) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO var_%s (id, value) VALUES (1, '%c') "
        "ON CONFLICT (id) DO UPDATE SET value = EXCLUDED.value;",
        name, val);
    exec_sql(sql);
}

char db_get_char(const char *name) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT value FROM var_%s WHERE id = 1;", name);
    check_conn();
    PGresult *res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        fprintf(stderr, "db_get_char: no value for var_%s\n", name);
        PQclear(res);
        exit(1);
    }
    char val = PQgetvalue(res, 0, 0)[0];
    PQclear(res);
    return val;
}

/* ── char* (TEXT) ── */

void db_ensure_table_str(const char *name) {
    char sql[512];
    snprintf(sql, sizeof(sql),
        "CREATE TABLE IF NOT EXISTS var_%s "
        "(id INTEGER PRIMARY KEY, value TEXT NOT NULL);",
        name);
    exec_sql(sql);
}

void db_set_str(const char *name, const char *val) {
    /* Escape via PQescapeLiteral to avoid injection */
    check_conn();
    char *escaped = PQescapeLiteral(conn, val, strlen(val));
    char sql[1024];
    snprintf(sql, sizeof(sql),
        "INSERT INTO var_%s (id, value) VALUES (1, %s) "
        "ON CONFLICT (id) DO UPDATE SET value = EXCLUDED.value;",
        name, escaped);
    PQfreemem(escaped);
    exec_sql(sql);
}

char *db_get_str(const char *name) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT value FROM var_%s WHERE id = 1;", name);
    check_conn();
    PGresult *res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        fprintf(stderr, "db_get_str: no value for var_%s\n", name);
        PQclear(res);
        exit(1);
    }
    char *val = strdup(PQgetvalue(res, 0, 0));
    PQclear(res);
    return val;
}
