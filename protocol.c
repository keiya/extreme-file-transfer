#include "protocol.h"
#include "stdio.h"
#include "utility.h"
int parse_command(FILE *in)
{
	char cmd[MFTP_HEADER_BUFFERSIZE];
	fgets(cmd,MFTP_HEADER_BUFFERSIZE,in);

	int command = 0;
	if (cmd[0] == 'q')
	{
		if (strstr(cmd,"qget") == cmd)
		{
			command = MFTP_CMD_GET;
		}
		if (strstr(cmd,"qput") == cmd)
		{
			command = MFTP_CMD_PUT;
		}
	}
	else if (cmd[0] == 's')
	{
		if (cmd[1] == '2')
		{
			command = MFTP_RES_SUCCESS;
		}
		else if (cmd[1] == '4')
		{
			command = MFTP_RES_FAIL;
		}
	}

	return command;
}

/*
struct header_entry* parse_headers(FILE *in)
{
	struct header_entry *headers = NULL;

	char headcnts[MFTP_HEADER_BUFFERSIZE];
	fgets(headcnts,MFTP_HEADER_BUFFERSIZE,in);
	int headcnt = 0;

	headcnt = atoi(headcnts);

	int i;
	for (i=0; i<headcnt;i++)
	{
		char requestheader[MFTP_HEADER_BUFFERSIZE];
		fgets(requestheader,MFTP_HEADER_BUFFERSIZE,in);
		char *headvalue = split(requestheader,':');
		chomp(headvalue);

		struct header_entry *item = malloc(sizeof(struct header_entry)); // hashtable
		strncpy(item->name, requestheader, 512);
		strncpy(item->value, headvalue, 512);
		HASH_ADD_STR(headers, name, item);
	}
	return headers;
}
*/

struct header_entry* parse_headers(FILE *in)
{
	struct header_entry *headers = NULL;

	int i;
	while (1)
	{
		char requestheader[MFTP_HEADER_BUFFERSIZE];
		fgets(requestheader,MFTP_HEADER_BUFFERSIZE,in);
		if (strstr(requestheader,"\r\n") == requestheader)
			break;

		char *headvalue = split(requestheader,':');
		if (headvalue == NULL) break;
		chomp(headvalue);

		struct header_entry *item = malloc(sizeof(struct header_entry)); // hashtable
		strncpy(item->name, requestheader, 512);
		strncpy(item->value, headvalue, 512);
		HASH_ADD_STR(headers, name, item);
	}
	return headers;
}


char *get_header_value(struct header_entry *header, char *header_name) {
	struct header_entry *tmp = malloc(sizeof(struct header_entry)); // hashtable

    HASH_FIND_STR( header, header_name, tmp );
	if (!tmp)
		return NULL;
    return tmp->value;
}
void create_header(char *buf,const char *cmd,struct header_entry *headers)
{
	struct header_entry *s;
	char tmp[MFTP_HEADER_BUFFERSIZE];
	snprintf(buf,MFTP_HEADER_BUFFERSIZE,
				"%s\r\n",cmd);
	if (headers == NULL) return;
    for(s=headers; s != NULL; s=s->hh.next) {
        snprintf(tmp,MFTP_HEADER_BUFFERSIZE,
				"%s:%s\r\n",s->name, s->value);
		strcat(buf,tmp);
    }
	snprintf(tmp,MFTP_HEADER_BUFFERSIZE,"\r\n");
	strcat(buf,tmp);
}
