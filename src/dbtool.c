#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

static int f_callback(void *, int, char **, char **);
static int t_callback(void *, int, char **, char **);
static int m_callback(void *, int, char **, char **);


static int
f_callback(void *data, int argc, char **argv, char **azColname)
{
	(void)data;
	int i;
	for (i = 0; i < argc; i++) {
		printf("%s\t", argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

static int
t_callback(void *data, int argc, char **argv, char **azColname)
{
	int i;
	int rc;
	char *zErrMsg = 0;
	char sql[1000];
	sprintf(sql, "SELECT tag FROM tagmap WHERE file=\'%s\'", azColname[0]);
	
	printf("file: %s\n", azColname[0]);
	rc = sqlite3_exec((sqlite3 *)data, sql, m_callback, NULL, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	return 0;
}

static int
m_callback(void *data, int argc, char **argv, char **azColname)
{
	(void)data;
	int i;
	for (i = 0; i < argc; i++) {
		printf("\t:%s\n", argv[i] ? argv[i] : "NULL");
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	sqlite3 *db;
	sqlite3_stmt *stmt;
	char *zErrMsg = 0;
	char *sql = "SELECT * FROM tagmap ORDER BY file";
	int rc;

	rc = sqlite3_open("tfs.db", &db);

	if (rc) {
		fprintf(stderr, "%s\n", sqlite3_errmsg(db));
		return 0;
	}
	
	printf("TABLE files\nname\n");
	rc = sqlite3_exec(db, "SELECT * FROM files;", f_callback, NULL, &zErrMsg);
	
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	printf("\n--------------------\n\n");

	printf("TABLE tags\nname\n");
	rc = sqlite3_exec(db, "SELECT * FROM tags;", f_callback, NULL, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	printf("\n--------------------\n\n");
	
	printf("Files and tags\n");
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
	}
	rc = sqlite3_step(stmt);
	const unsigned char *file = sqlite3_column_text(stmt, 0);
	const unsigned char *tag = sqlite3_column_text(stmt, 1);
	printf("file: %s\n", file);
	printf("\t:%s\n", tag);
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		tag = sqlite3_column_text(stmt, 1);
		printf("\t:%s\n", tag);
	}
	if (rc != SQLITE_DONE) {
		printf("Error: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);

	sqlite3_close(db);
	return 0;
}
