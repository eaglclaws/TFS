#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sqlite3.h>

int find(char **array, int size, const char *target) {
	for (int i = 0; i < size; i++) {
		if (strcmp(array[i], target) == 0) {
			return i;
		}
	}
	return -1;
}

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
	char **files;
	int **map;
	int filecount;
	struct stat st;

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
	map = malloc(tagcount * sizeof(int *));
	sqlite3_prepare_v2(db, "SELECT name FROM tags;", -1, &stmt, NULL);
	int i = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const char *tmp = (const char *)sqlite3_column_text(stmt, 0);
		tags[i] = malloc((strlen(tmp) + 1) * sizeof(char));
		strcpy(tags[i], tmp);
		i++;
	}
	sqlite3_finalize(stmt);

	sqlite3_prepare_v2(db, "SELECT COUNT(name) FROM files;", -1, &stmt, NULL);
	sqlite3_step(stmt);
	filecount = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	files = malloc(filecount * sizeof(char *));
	sqlite3_prepare_v2(db, "SELECT name FROM files;", -1, &stmt, NULL);
	i = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const char *tmp = (const char *)sqlite3_column_text(stmt, 0);
		files[i] = malloc((strlen(tmp) + 1) * sizeof(char));
		strcpy(files[i], tmp);
		i++;
	}
	sqlite3_finalize(stmt);

	for (int i = 0; i < tagcount; i++) {
		char sql[100];
		sprintf(sql, "SELECT COUNT(file) FROM tagmap WHERE tag=\'%s\';", tags[i]);
		sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
		sqlite3_step(stmt);
		int sz = sqlite3_column_int(stmt, 0);
		map[i] = malloc((sz + 1) * sizeof(int));
		map[i][0] = sz;
		sqlite3_finalize(stmt);

		sprintf(sql, "SELECT file FROM tagmap WHERE tag=\'%s\';", tags[i]);
		sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
		int j = 1;
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			const char *tmp = (const char *)sqlite3_column_text(stmt, 0);
			int index = find(files, filecount, tmp);
			if (index == -1) {
				continue;
			}
			map[i][j] = index;
			j++;
		}
		sqlite3_finalize(stmt);
	}

	/*
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
		filecount[i] = j;
		sqlite3_finalize(stmt);
	}*/
	if (argc == 1) {
		for (int i = 0; i < tagcount; i++) {
			printf("TAG: %s\n FILES\n", tags[i]);
			for (int j = 1; j <= map[i][0]; j++) {
				char buff[100];
				stat(files[map[i][j]], &st);
				strftime(buff, sizeof(buff), "%D", gmtime(&st.st_mtim.tv_sec));
				strftime(buff, sizeof(buff), "%D", gmtime(&st.st_mtim.tv_sec));
				printf(" ");
				printf( (st.st_mode & S_IRUSR) ? "r" : "-");
				printf( (st.st_mode & S_IWUSR) ? "w" : "-");
				printf( (st.st_mode & S_IXUSR) ? "x" : "-");
				printf( (st.st_mode & S_IRGRP) ? "r" : "-");
				printf( (st.st_mode & S_IWGRP) ? "w" : "-");
				printf( (st.st_mode & S_IXGRP) ? "x" : "-");
				printf( (st.st_mode & S_IROTH) ? "r" : "-");
				printf( (st.st_mode & S_IWOTH) ? "w" : "-");
				printf( (st.st_mode & S_IXOTH) ? "x" : "-");
				printf(" ");
				printf("%ld", st.st_size);
				printf(" ");
				printf("%s", buff);
				printf(" ");
				printf("%s\n", files[map[i][j]]);
			}
		}
	} else {
		char counter[1000] = "SELECT COUNT(name) FROM tags WHERE ";
		for (int i = 1; i < argc; i++) {
			char buff[100];
			sprintf(buff, "name=\'%s\'", argv[i]);
			strcat(counter, buff);
			if (i != argc - 1) {
				strcat(counter, " OR ");
			}
		}
		sqlite3_prepare_v2(db, counter, -1, &stmt, NULL);
		sqlite3_step(stmt);
		if (argc - 1 != sqlite3_column_int(stmt, 0)) {
			fprintf(stderr, "A tag does not exist\n");
			sqlite3_finalize(stmt);
			return -1;
		}
		sqlite3_finalize(stmt);
	
		char query[1000] = "SELECT file FROM tagmap GROUP BY file HAVING ";
		printf("TAG: ");
		for (int i = 1; i < argc; i++) {
			printf("%s ", argv[i]);
			char buff[100];
			sprintf(buff, "SUM(CASE WHEN tag=\'%s\' THEN 1 ELSE 0 END) > 0", argv[i]);
			strcat(query, buff);
			if (i != argc - 1) {
				strcat(query, " AND ");
			}
		}
		strcat(query, ";");
		printf("\n");
		sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
		i = 0;
		int pcount;
		while(sqlite3_step(stmt) == SQLITE_ROW) {
			i++;
		}
		pcount = i;
		char **pf = malloc(i * sizeof(char *));
		sqlite3_finalize(stmt);
	
		sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
		i = 0;
		while(sqlite3_step(stmt) == SQLITE_ROW) {
			const char *tmp = (const char *)sqlite3_column_text(stmt, 0);
			pf[i] = malloc((strlen(tmp) + 1) * sizeof(char));
			strcpy(pf[i], tmp);
			i++;
		}
		sqlite3_finalize(stmt);

		for (int i = 0; i < pcount; i++) {
			char buff[100];
			stat(pf[i], &st);
			strftime(buff, sizeof(buff), "%D", gmtime(&st.st_mtim.tv_sec));
			strftime(buff, sizeof(buff), "%D", gmtime(&st.st_mtim.tv_sec));
			printf(" ");
			printf( (st.st_mode & S_IRUSR) ? "r" : "-");
			printf( (st.st_mode & S_IWUSR) ? "w" : "-");
			printf( (st.st_mode & S_IXUSR) ? "x" : "-");
			printf( (st.st_mode & S_IRGRP) ? "r" : "-");
			printf( (st.st_mode & S_IWGRP) ? "w" : "-");
			printf( (st.st_mode & S_IXGRP) ? "x" : "-");
			printf( (st.st_mode & S_IROTH) ? "r" : "-");
			printf( (st.st_mode & S_IWOTH) ? "w" : "-");
			printf( (st.st_mode & S_IXOTH) ? "x" : "-");
			printf(" ");
			printf("%ld", st.st_size);
			printf(" ");
			printf("%s", buff);
			printf(" ");
			printf("%s\n", pf[i]);
		}

	}
	sqlite3_close(db);
	return 0;
}
