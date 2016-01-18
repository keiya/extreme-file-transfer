#include "stdio.h"

#ifndef INC_UTHASH_H
#define INC_UTHASH_H
#include "uthash/include/uthash.h"
#endif

#define MFTP_CMD_GET 2
#define MFTP_CMD_PUT 3
#define MFTP_CMD_CD 4
#define MFTP_CMD_DIR 5
#define MFTP_CMD_ABOR 6

#define MFTP_RES_SUCCESS 1
#define MFTP_RES_FAIL -1

#define	MFTP_HEADER_BUFFERSIZE	1024
#define	MFTP_HEADER_BUFFERSIZE_HALF	512

struct header_entry {
	char name[MFTP_HEADER_BUFFERSIZE_HALF];
	char value[MFTP_HEADER_BUFFERSIZE_HALF];
	UT_hash_handle hh;
};

int parse_command(FILE *);
struct header_entry* parse_headers(FILE *);
char *get_header_value(struct header_entry *, char *);
void create_header(char *,const char *,struct header_entry *);
