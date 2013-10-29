#include <stdio.h>
#include <libgen.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <sqlite3.h>
#include "common.h"

//the type of recevier
#define PERSON	"0"
#define GROUP	"1"

//the receivers node
struct receivers{
	char *name;
	struct receivers *next;
};


int revList(char **argv, struct receivers **list);
void delMem(struct receivers *list);
int insert2db(char *sender, char *file, const struct receivers *list, time_t curTime, off_t size);


/*
 argv[1]=the pusher, argv[2]=file,
 argv[3]=the type of first receiver, argv[4]=1st receiver
 argv[5]=the type of first receiver, argv[6]=1st receiver
 */
int main(int argc, char *argv[])
{
	struct receivers *list = NULL;
	struct stat fileStat;
	time_t curTime;
	char *file;
	int timeLen;
	char *cmd;//the copy command

	if(argc < 5 || argc%2 != 1)
	{
		printf("use the correct args\n");
		printf("like as: push user1 test.txt 0 user2 1 group1\n");
		return -1;
	}
	if(lstat(argv[2], &fileStat) != 0)//the file is a regular file?
	{
		perror("lstat failed\n");
		return -1;
	}
	if(!S_ISREG(fileStat.st_mode))
	{
		printf("it's not a regular file\n");
		return -1;
	}

	curTime = time(NULL);//change the file name in order to avoid the duplication of name
	timeLen = (int)log10f((float)curTime) + 1;
	file = (char *)malloc(sizeof(TMP)+ sizeof(basename(argv[2])) + timeLen);
	sprintf(file, "%s%s%d", TMP, basename(argv[2]), (int)curTime);

	//get the list of receives
	if(-1 == revList(argv, &list))
		return -1;

	if(insert2db(argv[1], file, list, curTime, fileStat.st_size)) //insert the data to db
		return -1;

	//copy the  file
	cmd = (char *)malloc(sizeof(argv[2])*2+timeLen+4);
	sprintf(cmd, "cp %s %s", argv[2], file);
//printf("%s\n", cmd);
	system(cmd);

	//free the memory
	delMem(list);
	free(file);
	free(cmd);

	return 0;
}

/*
 return the list of the receivers, especially for group
 */
int revList(char **argv, struct receivers **list)
{
	int i = 3;
	int recvLen = sizeof(struct receivers);
	struct receivers *user;
	char *groupMember = NULL;
	char *tmp;//save the member of group

	while(NULL != argv[i])
	{
		user = (struct receivers *)malloc(sizeof(recvLen));

		//let it be a single linked list
		user->next = *list;
		*list = user;

		if(0 == strcmp(argv[i], PERSON))//push the file to one person
		{
			i++;
			user->name = (char *)malloc(sizeof(argv[i]));
			strcpy(user->name, argv[i]);
		}
		else if(0 == strcmp(argv[i], GROUP))//push the file to a group
		{
			i++;
			//groupMember = getGroupMember(argv[i]);
			groupMember = (char *)malloc(100);
			strcpy(groupMember, "usr3 usr4");
			if((tmp = strtok(groupMember, " ")) == NULL)//it's a empty gruop
			{	i++;continue; }
			while(1)
			{
				user->name = (char *)malloc(strlen(tmp) + 1);
				strncpy(user->name, tmp, strlen(tmp) + 1);
				//user->name = tmp;
				//free(tmp);

				if((tmp = strtok(NULL, " ")) == NULL)//has no more member
					break;
			
				user = (struct receivers *)malloc(sizeof(recvLen));
				user->next = *list;//insert into the list
				*list = user;
			}
		}
		else//an unexpectd type
		{
			printf("there is a type except PERSON AND GROUP?\n");
			return -1;
		}

		i++;
	}

	return 0;
}

/*
 make sql and insert the data to db
 */
int insert2db(char *sender, char *file, const struct receivers *list, time_t curTime, off_t size)
{
	sqlite3 *db = NULL;
	//char sql[200];//maybe thers is a better way to do this
	char *sql;
	enum status stat = NOTSEND;
	char *errmsg;
	int cnt = 0;
	//int rc = 0;

	if(sqlite3_open(DB_NAME, &db) != SQLITE_OK)//open the db
	{
		printf("%s\n", sqlite3_errmsg(db));
		return -1;
	}

//insert the info table
	do{//at least list has one member
		sql = sqlite3_mprintf("INSERT INTO %s VALUES('%s', '%s', '%s', %d, %d, %d);", TABLE_NAME, sender, list->name, basename(file), (int)curTime, (int)size, stat);

		if(SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, &errmsg))//we don't care about the result
		{
			printf("%s\n", errmsg);
			sqlite3_close(db);
			return -1;
		}
		cnt++;//the file counter
		list = list->next;
		sqlite3_free(sql);
	}while(NULL != list);

//insert the file table
	sql = sqlite3_mprintf("insert into %s values('%s', '%d');", FILE_TABLE, basename(file), cnt);
	if(SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL))
	{
		printf("%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}
	sqlite3_free(sql);

	sqlite3_close(db);//close the db

	return 0;
}

/*
 clean the memory
 */
void delMem(struct receivers *list)
{
	struct receivers *tmp;

	while(list != NULL)
	{
		free(list->name);
		tmp = list;
		list = list->next;
		free(tmp);
	}
}
