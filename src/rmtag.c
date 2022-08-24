#include <stdio.h>
#include <sqlite3.h>

void
rmtag_usage()
{
	fprintf(stderr, "Usage: rmtag TAG...\n");
}

int
main(int argc, char *argv[])
{
	int rc;
	char *zErrMsg;
	char sql[1000];
	FILE *dbfile;
	sqlite3 *db;

	if (argc == 1) {
		rmtag_usage();
		return 0;
	}

	dbfile = fopen("../tfs.db", "r");
	if (dbfile == NULL) {
		fprintf(stderr,
			"Database does not exist, are you sure you are in a tfs directory?\n");
		return 0;
	}
	fclose(dbfile);

	rc = sqlite3_open("../tfs.db", &db);
	if (rc) {
		fprintf(stderr,
				"Couldn't open database file: %s\n", sqlite3_errmsg(db));
		return 0;
	} else {
		//For debug only, remove block after release
		fprintf(stderr, "Database file opened\n");
	}

	for (int i = 1; i < argc; i++) {
		sprintf(sql, "PRAGMA foreign_keys = ON; DELETE FROM tags WHERE name=\'%s\';", argv[i]);
		rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
		if (rc) {
			fprintf(stderr, "%s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
	}

	sqlite3_close(db);
	return 0;
}
