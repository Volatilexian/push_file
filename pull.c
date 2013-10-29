#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "common.h"

int changeDB(const char *file, enum status stat);//return the time which after the basename
int rejectFile(const char *file);
int acceptFile(const char *file, int time, char *dir);
int decreaseCnt(const char *file);

/*
 use two or three args to call this
 like as: pull 0  file or pull 1 file [dir]
 */
int main(int argc, char *argv[])
{
	enum status stat;//accept or reject
	int time;
	char *dir;
	char ch;

	if(argc != 3 && argc != 4)
	{
		printf("usage: {0/1} filename [dir]\n");
		return -1;
	}

	ch = argv[1][0];

	switch(ch)
	{
	case '0'://reject the file
		stat = REJECT;
		//change the status in DB
		if(changeDB((const char *)argv[2], stat) < 0)
		{
			printf("can not change the info stat in DB\n");
			return -1;
		}
		rejectFile((const char *)argv[2]);
		break;
	case '1'://accept the file
		stat = ACCEPT;
		//change the status in DB
		if((time = changeDB((const char *)argv[2], stat)) < 0)
		{
			printf("can not change the info stat in DB\n");
			return -1;
		}

		//judge the dir is ok or not
		if(NULL == argv[3])
		{
			dir = (char *)malloc(sizeof(DEST));
			strcpy(dir, DEST);
		}
		else //whether the argv[3] is invalid?
		{
			dir = (char *)malloc(sizeof(argv[3]));
			strcpy(dir, argv[3]);
		}

		acceptFile((const char *)argv[2], time, dir);
		break;
	default:
		printf("bad stat\n");
		return -1;
	}
	
	if(ACCEPT == stat)
		free(dir);

	return 0;
}

/*
 change the stat in db to ACCEPT OR REJECT
 if reject the file, return 0; if accept the file, return the time
 else return value is wrong
 */
int changeDB(const char *file, enum status stat)
{
	int rc;
	int time = 0;
	char *sql;
	//char *tail;
	//char buffer[100];

	sqlite3_stmt *stmt = NULL;
	sqlite3 *db = NULL;

	rc = sqlite3_open(DB_NAME, &db);
	if(rc != SQLITE_OK)
	{
		printf("%s\n", sqlite3_errmsg(db));
		return -1;
	}

	//sql = (char *)malloc(sizeof("update  set stat='' where file='';") + sizeof(TABLE_NAME) + 1 + sizeof(file) - 2);//need '\0' only one
	sql = sqlite3_mprintf("update %s set stat='%d' where file='%s';", TABLE_NAME, stat, file);
	//sql = sqlite3_mprintf("update %s set stat='%d' where file='%s'; select * from %s where file='%s';", TABLE_NAME, stat, file, TABLE_NAME, file);

	//memset(buffer, 0, 100);
	//if(sqlite3_prepare(db, "select * from test;", -1, &stmt, NULL) != SQLITE_OK)
	if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
	{
		printf("sqlite3_prepare_v2 failed\n");
		printf("%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}
	if(sqlite3_step(stmt) != SQLITE_DONE)
		printf("there must be wrong\n");

	sqlite3_free(sql);
	sqlite3_finalize(stmt);

//get the time
	if(ACCEPT == stat)
	{
		sql = sqlite3_mprintf("select * from %s where file='%s';", TABLE_NAME, file);
		if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
		{
			printf("sqlite3_prepare_v2 failed\n");
			printf("%s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			return -1;
		}
	
		if(sqlite3_step(stmt) != SQLITE_ROW)
		//printf("%d\n", sqlite3_step(stmt));
		//printf("%d\n", sqlite3_column_count(stmt));
			printf("there must be wrong\n");
		time = sqlite3_column_int(stmt, 3);
		//printf("%d\n", time);

/*	if(SQLITE_OK != sqlite3_get_table(db, sql, &result, &rows, &cols, &errmsg))//i need the result
	{
		printf("%s\n", errmsg);
		return -1;
	}
*/
		sqlite3_free(sql);
		sqlite3_finalize(stmt);
	}

	sqlite3_close(db);

	//return atoi(result[(rows-1)*cols+4]);//return the time
	return time;//return the time
}

/*
 del the file
 decrease the counter of the file
 */
int rejectFile(const char *file)
{
	//char *cmd;
	//cmd = sqlite3_mprintf("rm %s%s", TMP, file);
	//system(cmd);
	//sqlite3_free(cmd);

	return decreaseCnt(file);
}

/*
 move the file to user space
 copy the file and decrease the counter of the file
 */
int acceptFile(const char *file, int time, char *dir)
{
	char *source;
	char *dest;
	char *cmd;
	char *file2;
	int timeLen;
	int len;

//get the file
	//get the destination file name
	timeLen = (int)log10((float)time) + 1;
	len = strlen(file) - timeLen + 1;
	file2 = (char *)malloc(len);
	strncpy(file2, file, len - 1);
	file2[len - 1] = '\0';

	source = sqlite3_mprintf("%s%s", TMP, file);

	if(NULL == dir)//use default dir
		dest = sqlite3_mprintf("%s%s", DEST, file2);
	else
		dest = sqlite3_mprintf("%s%s", dir, file2);

	cmd = sqlite3_mprintf("cp %s %s", source, dest);//don't remove the file
	system(cmd);

	sqlite3_free(cmd);
	sqlite3_free(dest);
	sqlite3_free(source);
	free(file2);

//decrease the counter of the file
	return decreaseCnt(file);
}

/*
 decrease the counter of the file which in the db
 */
int decreaseCnt(const char *file)
{
	sqlite3 *db = NULL;
	char *sql = NULL;
	int cnt = 0;
	sqlite3_stmt *stmt = NULL;

	if(sqlite3_open(DB_NAME, &db) != SQLITE_OK)
	{
		printf("%s\n", sqlite3_errmsg(db));
		return -1;
	}

	sql = sqlite3_mprintf("select * from %s where file='%s';", FILE_TABLE, file);
	if(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
	{
		printf("%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}
	if(sqlite3_step(stmt) != SQLITE_ROW)
		printf("there must be wrong\n");
	cnt = sqlite3_column_int(stmt, 1);
	cnt--;
	sqlite3_free(sql);
	sqlite3_finalize(stmt);

	sql = sqlite3_mprintf("update %s set cnt='%d' where file='%s';", FILE_TABLE, cnt, file);
	if(sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK)
	{
		printf("%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}

	sqlite3_free(sql);
	sqlite3_close(db);

	return 0;
}
