#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

void
mktag_usage()
{
	fprintf(stderr, "Usage: mktag TAG...\n");
}

int
main(int argc, char *argv[])
{
	FILE *dbfile;
	sqlite3 *db;
	int rc;
	char sql[1000];
	char *zErrMsg;

	if (argc == 1) {
		mktag_usage();
		return 0;
	}

	dbfile = fopen("../tfs.db", "r");
	if (dbfile == NULL) {
		fclose(dbfile);
		fprintf(stderr, 
				"Database file does not exist, \
				are you sure you are in a tfs directory?\n");
		return 0;
	}
	rc = sqlite3_open("../tfs.db", &db);
	if (rc) {
		fprintf(stderr, "Couldn't open database file: %s\n",
				sqlite3_errmsg(db));
		return 0;

	} else {
		//Debug only, remove else block after rollout
		fprintf(stderr, "Database file opened\n");
	}

	for (int i = 1; i < argc; i++) {
		sprintf(sql, "INSERT INTO tags VALUES(\'%s\');", argv[i]);
		rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
		if (rc) {
			fprintf(stderr, "%s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
	}

	sqlite3_close(db);
	return 0;
}
