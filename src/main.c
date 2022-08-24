#define FUSE_USE_VERSION 31

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE

#ifdef linux
#define _XOPEN_SOURCE 700
#endif

#define TFS_USER_DATA ((struct tfs_state *) fuse_get_context()->private_data)

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <limits.h>
#include <sqlite3.h>

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "sha256.h"

struct tfs_state {
	FILE *logfile;
	sqlite3 *dbfile;
	char *rootdir;

	char *curfile;
};

static int fill_dir_plus = 0;

void
tfs_fullpath(char fpath[PATH_MAX], const char *path)
{
	strcpy(fpath, TFS_USER_DATA->rootdir);
	strcat(fpath, path);
}

static void *
tfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	(void) conn;
	cfg->use_ino = 1;
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;
	fprintf(TFS_USER_DATA->logfile, "init\n");
	return TFS_USER_DATA;
}

static int
tfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
	(void) fi;
	int res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);
	res = lstat(fpath, stbuf);
	if (res == -1) return -errno;
	fprintf(TFS_USER_DATA->logfile, "getattr\n");
	return 0;
}

static int
tfs_access(const char *path, int mask)
{
	int res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);
	res = access(fpath, mask);
	if (res == -1) return -errno;
	fprintf(TFS_USER_DATA->logfile, "access\n");
	return 0;
}

static int
tfs_readlink(const char *path, char *buf, size_t size)
{
	int res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);
	res = readlink(fpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	fprintf(TFS_USER_DATA->logfile, "readlink\n");
	return 0;
}

static int
tfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
            struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
	DIR *dp;
	struct dirent *de;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);


	(void) offset;
	(void) fi;
	(void) flags;

	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0, fill_dir_plus))
			break;
	}

	closedir(dp);
	fprintf(TFS_USER_DATA->logfile, "readdir\n");
	return 0;
}

static int
tfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	char fpath[PATH_MAX];
	char rpath[PATH_MAX];
	char sha[65];
	tfs_fullpath(fpath, path);
	int dirfd = AT_FDCWD;
	if (S_ISREG(mode)) {
		res = openat(dirfd, fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISDIR(mode)) {
		res = mkdirat(dirfd, fpath, mode);
	} else if (S_ISFIFO(mode)) {
		res = mkfifoat(dirfd, fpath, mode);
	} else {
		res = mknodat(dirfd, fpath, mode, rdev);
	}
	sha256_file(fpath, sha);
	tfs_fullpath(rpath, sha);
	fprintf(TFS_USER_DATA->logfile, "mknod file %s with hash %s\n", path, sha);
	if (res == -1)
		return -errno;
	fprintf(TFS_USER_DATA->logfile, "mknod\n");
	return 0;
}

static int
tfs_mkdir(const char *path, mode_t mode)
{
	int res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);

	res = mkdir(fpath, mode);
	if (res == -1)
		return -errno;

	fprintf(TFS_USER_DATA->logfile, "mkdir\n");
	return 0;
}

static int
tfs_unlink(const char *path)
{
	int res;
	char fpath[PATH_MAX];
	char sql[1000];
	char *zErrMsg = 0;
	tfs_fullpath(fpath, path);

	res = unlink(fpath);
	if (res == -1)
		return -errno;
	sprintf(sql, 
		"DELETE FROM files where name=\'%s\'",
		path[0] == '/' ? path + 1 : path
	);
	int rc = sqlite3_exec(TFS_USER_DATA->dbfile, sql, NULL, 0, &zErrMsg);
	fprintf(TFS_USER_DATA->logfile, "%s\n", zErrMsg);
	sqlite3_free(zErrMsg);
	fprintf(TFS_USER_DATA->logfile, "unlink\n");
	return 0;
}

static int
tfs_rmdir(const char *path)
{
	int res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);

	res = rmdir(fpath);
	if (res == -1)
		return -errno;

	fprintf(TFS_USER_DATA->logfile, "rmdir\n");
	return 0;
}

static int
tfs_symlink(const char *from, const char *to)
{
	int res;
	char ffrom[PATH_MAX];
	tfs_fullpath(ffrom, from);
	char fto[PATH_MAX];
	tfs_fullpath(fto, to);
	res = symlink(ffrom, fto);
	if (res == -1)
		return -errno;

	fprintf(TFS_USER_DATA->logfile, "symlink\n");
	return 0;
}

static int
tfs_rename(const char *from, const char *to, unsigned int flags)
{
	int res;
	char ffrom[PATH_MAX];
	tfs_fullpath(ffrom, from);
	char fto[PATH_MAX];
	tfs_fullpath(fto, to);
	
	if (flags)
		return -EINVAL;

	res = rename(ffrom, fto);
	if (res == -1)
		return -errno;
	fprintf(TFS_USER_DATA->logfile, "rename %s to %s\n", from, to);
	return 0;
}

static int
tfs_link(const char *from, const char *to)
{
	int res;
	char ffrom[PATH_MAX];
	tfs_fullpath(ffrom, from);
	char fto[PATH_MAX];
	tfs_fullpath(fto, to);

	res = link(ffrom, fto);
	if (res == -1)
		return -errno;

	fprintf(TFS_USER_DATA->logfile, "link\n");
	return 0;
}

static int
tfs_chmod(const char *path, mode_t mode,
		     struct fuse_file_info *fi)
{
	(void) fi;
	int res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);

	res = chmod(fpath, mode);
	if (res == -1)
		return -errno;

	fprintf(TFS_USER_DATA->logfile, "chmod\n");
	return 0;
}

static int
tfs_chown(const char *path, uid_t uid, gid_t gid,
		     struct fuse_file_info *fi)
{
	(void) fi;
	int res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);

	res = lchown(fpath, uid, gid);
	if (res == -1)
		return -errno;

	fprintf(TFS_USER_DATA->logfile, "chown\n");
	return 0;
}

static int
tfs_truncate(const char *path, off_t size,
			struct fuse_file_info *fi)
{
	int res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);

	if (fi != NULL)
		res = ftruncate(fi->fh, size);
	else
		res = truncate(fpath, size);
	if (res == -1)
		return -errno;

	fprintf(TFS_USER_DATA->logfile, "truncate\n");
	return 0;
}

static int
tfs_utimens(const char *path, const struct timespec ts[2],
		       struct fuse_file_info *fi)
{
	(void) fi;
	int res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, fpath, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	fprintf(TFS_USER_DATA->logfile, "utimens\n");
	return 0;
}

static int
tfs_create(const char *path, mode_t mode,
		      struct fuse_file_info *fi)
{
	int res;
	char fpath[PATH_MAX];
	char rpath[PATH_MAX];
	char sha[65];
	char shapath[66];
	shapath[0] = '/';
	shapath[1] = 0;
	tfs_fullpath(fpath, path);
	TFS_USER_DATA->curfile = malloc(PATH_MAX * sizeof(char));
	strcpy(TFS_USER_DATA->curfile, path);
	fprintf(TFS_USER_DATA->logfile, "%s in state\n", TFS_USER_DATA->curfile);
	res = open(fpath, fi->flags, mode);
	sha256_file(fpath, sha);
	strcat(shapath, sha);
	tfs_fullpath(rpath, shapath);
	fprintf(TFS_USER_DATA->logfile, "create file %s with hash %s\n",
			fpath, rpath);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

static int
tfs_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	char fpath[PATH_MAX];
	char rpath[PATH_MAX];
	char sha[65];
	tfs_fullpath(fpath, path);

	res = open(fpath, fi->flags);
	sha256_file(fpath, sha);
	tfs_fullpath(rpath, sha);
	fprintf(TFS_USER_DATA->logfile, "open file %s with hash %s\n", path, sha);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

static int
tfs_read(const char *path, char *buf, size_t size, off_t offset,
         struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fpath[PATH_MAX];
	char rpath[PATH_MAX];
	char sha[65];
	tfs_fullpath(fpath, path);

	if(fi == NULL)
		fd = open(fpath, O_RDONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	sha256_file(fpath, sha);
	tfs_fullpath(rpath, sha);
	fprintf(TFS_USER_DATA->logfile, "read file %s with hash %s\n", path, sha);
	return res;
}

static int
tfs_write(const char *path, const char *buf, size_t size,
          off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fpath[PATH_MAX];
	char rpath[PATH_MAX];
	char sha[65];
	tfs_fullpath(fpath, path);

	(void) fi;
	if(fi == NULL)
		fd = open(fpath, O_WRONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	sha256_file(fpath, sha);
	tfs_fullpath(rpath, sha);
	TFS_USER_DATA->curfile = malloc(PATH_MAX * sizeof(char));
	strcpy(TFS_USER_DATA->curfile, path);
	fprintf(TFS_USER_DATA->logfile, "%s in state\n", TFS_USER_DATA->curfile);
	fprintf(TFS_USER_DATA->logfile, "write file %s with hash %s\n", path, sha);
	return res;
}

static int
tfs_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);

	res = statvfs(fpath, stbuf);
	if (res == -1)
		return -errno;

	fprintf(TFS_USER_DATA->logfile, "statfs\n");
	return 0;
}

static int
tfs_release(const char *path, struct fuse_file_info *fi)
{
	char fpath[PATH_MAX];
	char rpath[PATH_MAX];
	char *filename;
	char *token;
	char sql[1000];
	char *zErrMsg = 0;
	int rc;
	const char *delimit = ".";
	char sha[65];
	char shapath[66];
	shapath[0] = '/';
	shapath[1] = 0;
	tfs_fullpath(fpath, path);
	sha256_file(fpath, sha);
	strcat(shapath, sha);
	tfs_fullpath(rpath, shapath);
	if (TFS_USER_DATA->curfile != NULL) {
		filename = TFS_USER_DATA->curfile + 1;
		token = strtok(filename, delimit);
		token = strtok(NULL, delimit);
	}
	fprintf(TFS_USER_DATA->logfile, "release file %s with hash %s\n", path, sha);
	if (strcmp(fpath, rpath) != 0) {
		sqlite3_stmt *stmt;
		rename(fpath, rpath);
		char temp[1000];
		sprintf(temp, "SELECT name FROM files WHERE name=\'%s\';", path[0] == '/' ? path + 1 : path);
		sqlite3_prepare_v2(TFS_USER_DATA->dbfile, temp, -1, &stmt, NULL);
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_ROW) {
			sqlite3_finalize(stmt);
			sprintf(sql, "INSERT INTO files VALUES(\'%s\');", sha);
		} else {
			sqlite3_finalize(stmt);
			sprintf(sql,
				"UPDATE files SET name=\'%s\' WHERE name=\'%s\';",
				sha, path[0] == '/' ? path + 1 : path);
		}
		fprintf(TFS_USER_DATA->logfile, "%s\n", sql);
		rc = sqlite3_exec(TFS_USER_DATA->dbfile, sql, NULL, 0, &zErrMsg);
		fprintf(TFS_USER_DATA->logfile, "%p %d %d %s\n", TFS_USER_DATA->dbfile, SQLITE_OK, rc, zErrMsg);
		sqlite3_free(zErrMsg);
	}
	if (TFS_USER_DATA->curfile != NULL) {
		filename = NULL;
		token = NULL;
		TFS_USER_DATA->curfile = NULL;
		free(TFS_USER_DATA->curfile);
	}
	close(fi->fh);
	return 0;
}

static int
tfs_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int
tfs_fallocate(const char *path, int mode, off_t offset, off_t length,
              struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);

	(void) fi;

	if (mode)
		return -EOPNOTSUPP;

	if(fi == NULL)
		fd = open(fpath, O_WRONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = -posix_fallocate(fd, offset, length);

	if(fi == NULL)
		close(fd);
	return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int
tfs_setxattr(const char *path, const char *name, const char *value,
             size_t size, int flags)
{
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);
	int res = lsetxattr(fpath, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int
tfs_getxattr(const char *path, const char *name, char *value, size_t size)
{
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);
	int res = lgetxattr(fpath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int
tfs_listxattr(const char *path, char *list, size_t size)
{
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);
	int res = llistxattr(fpath, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int
tfs_removexattr(const char *path, const char *name)
{
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);
	int res = lremovexattr(fpath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

#ifdef HAVE_COPY_FILE_RANGE
static ssize_t
tfs_copy_file_range(const char *path_in, struct fuse_file_info *fi_in,
                    off_t offset_in, const char *path_out,
					struct fuse_file_info *fi_out, off_t offset_out,
					size_t len, int flags)
{
	int fd_in, fd_out;
	ssize_t res;
	char fpath_in[PATH_MAX];
	tfs_fullpath(fpath_in, path_in);
	char fpath_out[PATH_MAX];
	tfs_fullpath(fpath_out, path_out);

	if(fi_in == NULL)
		fd_in = open(fpath_in, O_RDONLY);
	else
		fd_in = fi_in->fh;

	if (fd_in == -1)
		return -errno;

	if(fi_out == NULL)
		fd_out = open(fpath_out, O_WRONLY);
	else
		fd_out = fi_out->fh;

	if (fd_out == -1) {
		close(fd_in);
		return -errno;
	}

	res = copy_file_range(fd_in, &offset_in, fd_out, &offset_out, len,
			      flags);
	if (res == -1)
		res = -errno;

	if (fi_out == NULL)
		close(fd_out);
	if (fi_in == NULL)
		close(fd_in);

	return res;
}
#endif

static off_t
tfs_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi)
{
	int fd;
	off_t res;
	char fpath[PATH_MAX];
	tfs_fullpath(fpath, path);

	if (fi == NULL)
		fd = open(fpath, O_RDONLY);
	else
		fd = fi->fh;

	if (fd == -1)
		return -errno;

	res = lseek(fd, off, whence);
	if (res == -1)
		res = -errno;

	if (fi == NULL)
		close(fd);
	return res;
}

static const struct fuse_operations tfs_ops = {
	.init            = tfs_init,
	.getattr         = tfs_getattr,
	.access          = tfs_access,
	.readlink        = tfs_readlink,
	.readdir         = tfs_readdir,
	.mknod           = tfs_mknod,
	.mkdir           = tfs_mkdir,
	.symlink         = tfs_symlink,
	.unlink          = tfs_unlink,
	.rmdir           = tfs_rmdir,
	.rename          = tfs_rename,
	.link            = tfs_link,
	.chmod           = tfs_chmod,
	.chown           = tfs_chown,
	.truncate        = tfs_truncate,
	.utimens         = tfs_utimens,
	.open            = tfs_open,
	.create          = tfs_create,
	.read            = tfs_read,
	.write           = tfs_write,
	.statfs          = tfs_statfs,
	.release         = tfs_release,
	.fsync           = tfs_fsync,
#ifdef HAVE_POSIX_FALLOCATE
	.fallocate       = tfs_fallocate,
#endif
#ifdef HAVE_SETXATTR
	.setxattr        = tfs_setxattr,
	.getxattr        = tfs_getxattr,
	.listxattr       = tfs_listxattr,
	.removexattr     = tfs_removexattr,
#endif
#ifdef HAVE_COPY_FILE_RANGE
	.copy_file_range = tfs_copy_file_range,
#endif
	.lseek           = tfs_lseek,
};

void
tfs_usage(char *name)
{
	fprintf(stderr, "Usage: %s [FUSE and mount options] root_directory mount_point\n", name);
	abort();
}

int
main(int argc, char *argv[])
{
	//const char *foo = "2c26b46b68ffc68ff99b453c1d30413413422d706483bfa0f98a5e886266e7ae";
	//printf("%d\n", strcmp(sha256("foo"), foo));
	//char test[65];
	//sha256_file(argv[1], test);
	//printf("%s\n", test);
	struct tfs_state *tfs_data;
	FILE *log;
	FILE *tmp;
	char *zErrMsg;
	sqlite3 *db;
	int rc;
	log = fopen("tfs.log", "w");
	tmp = fopen("tfs.db", "r");
	if (tmp) {
		fclose(tmp);
		sqlite3_open("tfs.db", &db);
	} else {
		sqlite3_open("tfs.db", &db);
		char *sql =
			"CREATE TABLE files(" \
			"name VARCHAR PRIMARY KEY NOT NULL); " \
			"CREATE TABLE tags(" \
			"name VARCHAR PRIMARY KEY NOT NULL); " \
			"CREATE TABLE tagmap(" \
			"file VARCHAR NOT NULL," \
			"tag VARCHAR NOT NULL," \
			"FOREIGN KEY (file) REFERENCES files (name) \
			ON DELETE CASCADE ON UPDATE CASCADE,"\
			"FOREIGN KEY (tag) REFERENCES tags (name) \
			ON DELETE CASCADE ON UPDATE CASCADE," \
			"PRIMARY KEY (file, tag));";
		rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
		if( rc != SQLITE_OK ){
      		fprintf(stderr, "SQL error: %s\n", zErrMsg);
      		sqlite3_free(zErrMsg);
   		} else {
      		fprintf(stdout, "Tables created successfully\n");
   		}
	}
	sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, NULL);
	if (log == NULL) {
		perror("logfile");
		exit(EXIT_FAILURE);
	}
	setvbuf(log, NULL, _IOLBF, 0);
	if ((argc < 3) || (argv[argc - 2][0] == '-') || (argv[argc - 1][0] == '-'))
		tfs_usage(argv[0]);
	tfs_data = malloc(sizeof(struct tfs_state));
	if (tfs_data  == NULL) {
		perror("main malloc");
		abort();
	}
	tfs_data->rootdir = realpath(argv[argc - 2], NULL);
	argv[argc - 2] = argv[argc - 1];
	argv[argc - 1] = NULL;
	argc--;
	tfs_data->logfile = log;
	tfs_data->dbfile = db;
	return fuse_main(argc, argv, &tfs_ops, tfs_data);
}
