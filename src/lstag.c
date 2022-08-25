#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

int
main(int argc, char *argv[])
{
	int rc;
	char sql1[100];
	char sql2[100];
	sqlite3 *db;
	sqlite3_stmt *stmt;
	FILE *dbfile;
	int tagcount;
	char **tags;
	char ***files;

	dbfile = fopen("../tfs.db", "r");
	if (dbfile == NULL) {
		fclose(dbfile);
		fprintf(stderr, "Database not found, are you sure you are in tfs?\n");
		return -1;
	}
	fclose(dbfile);

	rc = sqlite3_open("../tfs.db", &db);
	if (rc) {
		//This should never happen
		fprintf(stderr, "Database open failed: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	
	sqlite3_prepare_v2(db, "SELECT COUNT(name) FROM tags;", -1, &stmt, NULL);
	sqlite3_step(stmt);
	tagcount = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	
	tags = malloc(tagcount * sizeof(char *));
	files = malloc(tagcount * sizeof(char *));
	sqlite3_prepare_v2(db, "SELECT name FROM tags;", -1, &stmt, NULL);
	int i = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const char *tmp = (const char *)sqlite3_column_text(stmt, 0);
		tags[i] = malloc((strlen(tmp) + 1) * sizeof(char));
		strcpy(tags[i], tmp);
		i++;
	}
	sqlite3_finalize(stmt);
	for (int i = 0; i < tagcount; i++) {
		char sql[100];
		int count = 0;
		sprintf(sql, "SELECT COUNT(file) FROM tagmap WHERE tag=\'%s\';", tags[i]);
		sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
		sqlite3_step(stmt);
		count = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);
		files[i] = malloc(count * sizeof(char *));

		sprintf(sql, "SELECT file FROM tagmap WHERE tag=\'%s\';", tags[i]);
		sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
		int j = 0;
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			const char *tmp = (const char *)sqlite3_column_text(stmt, 0);
			files[i][j] = malloc((strlen(tmp) + 1) * sizeof(char));
			strcpy(files[i][j], tmp);
			j++;
		}
		sqlite3_finalize(stmt);
	}
	sqlite3_close(db);
	return 0;
}
