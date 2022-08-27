#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

void
tag_usage()
{
	fprintf(stderr, "Usage: tag FILE TAG...");
}

int
main(int argc, char *argv[])
{
	char *zErrMsg;
	char sql[1000];
	int rc;
	sqlite3 *db;
	sqlite3_stmt *stmt;
	FILE *dbfile;

	if (argc < 3 || (argc > 2 && strlen(argv[1]) != 64)) {
		tag_usage();
	}

	dbfile = fopen("../tfs.db", "r");
	if (dbfile == NULL) {
		fclose(dbfile);
		fprintf(stderr, "Database not found, are you sure you are in tfs?\n");
		return -1;
	}
	fclose(dbfile);
	rc = sqlite3_open("../tfs.db", &db);

	dbfile = fopen(argv[1], "r");
	if (dbfile == NULL) {
		fclose(dbfile);
		fprintf(stderr, "File does not exist\n");
		return -1;
	}
	fclose(dbfile);

	char counter[1000] = "SELECT COUNT(name) FROM tags WHERE ";
	for (int i = 2; i < argc; i++) {
		char buff[100];
		sprintf(buff, "name=\'%s\'", argv[i]);
		strcat(counter, buff);
		if (i != argc - 1) {
			strcat(counter, " OR ");
		}
	}
	strcat(counter, ";");
	sqlite3_prepare_v2(db, counter, -1, &stmt, NULL);
	sqlite3_step(stmt);
	if (argc - 2 != sqlite3_column_int(stmt, 0)) {
		printf("A tag does not exist\n");
		return -1;
	}
	for (int i = 2; i < argc; i++) {
		sprintf(sql, "INSERT INTO tagmap VALUES(\'%s\', \'%s\');", argv[1], argv[i]);
		rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
		if (rc) {
			fprintf(stderr, "%s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
	}
	
	sqlite3_close(db);
	return 0;
}
