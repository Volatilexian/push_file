//the name of database
#define DB_NAME "./test.db"

//table in db
#define TABLE_NAME "test"//the push info table
#define FILE_TABLE "file"//the file counter table


//the tmp dir, it must followed by '/'
#define TMP		"./tmp/"

//the destination dir, it must followed by '/'
#define DEST		"./dest/"

//the stat of the push information
enum status { NOTSEND, ACCEPT, REJECT, TIMEOUT };
