#include "protocol.h"
#include "stdio.h"
#include "utility.h"
int parse_command(FILE *in)
{
	char cmd[MFTP_HEADER_BUFFERSIZE];
	int command = MFTP_FAIL;

	if ( fgets(cmd,MFTP_HEADER_BUFFERSIZE,in) == NULL )
	{
		return 0;
	}

	if (cmd[0] == 'q')
	{
		if (strstr(cmd,"qget") == cmd)
		{
			command = MFTP_CMD_GET;
		}
		else if (strstr(cmd,"qput") == cmd)
		{
			command = MFTP_CMD_PUT;
		}
		else if (strstr(cmd,"qdir") == cmd)
		{
			command = MFTP_CMD_DIR;
		}
		else if (strstr(cmd,"qcd") == cmd)
		{
			command = MFTP_CMD_CD;
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
			command = MFTP_FAIL;
		}
	}

	return command;
}

/*
void discard_headers(FILE *in)
{
	while ( fgets(requestheader,MFTP_HEADER_BUFFERSIZE,in) != NULL )
	{
		if (strstr(requestheader,"\r\n") == requestheader)
			break;
	}
}
*/

struct header_entry* parse_headers(FILE *in)
{
	struct header_entry *headers = NULL;

	char requestheader[MFTP_HEADER_BUFFERSIZE];
	while ( fgets(requestheader,MFTP_HEADER_BUFFERSIZE,in) != NULL )
	{
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
	char *string = malloc(strlen(tmp->value) + 1); // hashtable
	strcpy(string,tmp->value);	
	//free(tmp);
    return string;
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

void free_header(struct header_entry *he)
{
	if (he == NULL) return;
	struct header_entry *header, *tmp;
	HASH_ITER(hh, he, header, tmp) {
		//printf("FREE:%p @ %p\n",he,header);
		HASH_DEL(he,header);  /* delete; users advances to next */
		free(header);            /* optional- if you want to free  */
	}
}
