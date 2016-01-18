#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <sys/types.h>	/* socket() */
#include <sys/socket.h>	/* socket() */
#include <netinet/in.h>	/* struct sockaddr_in */
#include <netdb.h>	/* getaddrinfo() */
#include "rubgc.h"
#include "cwd.h"
#include "utility.h"
#include "protocol.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h> /* basename */

#define MAX_HISTORY_CNT 10

/* Garbage collector context */
void *gc_ctx;

char *cmd;

struct mftpc_conn_handle {
	FILE *in;
	FILE *out;
};

struct mftpc_conn_handle handle;

void sigint_action(int signum, siginfo_t *info, void *ctx)
{
#ifdef DEBUG
	printf("sigint_handler(%d): signo(%d) code(0x%x)\n",
		signum, info->si_signo, info->si_code);
#endif
}

void set_sighandler()
{
	struct sigaction sa_sigint;
	
	memset(&sa_sigint, 0, sizeof(sa_sigint));
	sa_sigint.sa_sigaction = sigint_action;
	sa_sigint.sa_flags = SA_RESTART | SA_SIGINFO;
	
	if (sigaction(SIGINT, &sa_sigint, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}
}

#define PORTNO_BUFSIZE 30
int
tcp_connect( char *server, int portno )
{
    struct addrinfo hints, *ai;
    char portno_str[PORTNO_BUFSIZE];
    int s, err;
	snprintf( portno_str,sizeof(portno_str),"%d",portno );
	memset( &hints, 0, sizeof(hints) );
	hints.ai_socktype = SOCK_STREAM;
	if( (err = getaddrinfo( server, portno_str, &hints, &ai )) )
	{
	    fprintf(stderr,"unknown server %s (%s)\n",server,
		    gai_strerror(err) );
	    goto error0;
	}
	if( (s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0 )
	{
	    perror("socket");
	    goto error1;
	}
	if( connect(s, ai->ai_addr, ai->ai_addrlen) < 0 )
	{
	    perror( server );
	    goto error2;
	}
	freeaddrinfo( ai );
	return( s );
error2:
	close( s );
error1:
	freeaddrinfo( ai );
error0:
	return( -1 );
}

void show_prompt(char *buf)
{
	char cwd_str[128];
	getcwd(&cwd_str,128);
	snprintf(buf,128,"[%s](mFTP)> ",cwd_str);
}

#define	BUFFERSIZE 4096
int mftpc_conn(char *server, int portno)
{

	int sock = tcp_connect( server, portno );
	if( sock<0 )
		return 0;
	    //exit( 1 );
	if( fdopen_sock(sock,&handle.in,&handle.out) < 0 )
	{
	    fprintf(stderr,"fdooen()\n");
	    exit( 1 );
	}
	return 1;
}
void mftpc_get(const char *param)
{
	char *header_buf = (char *)malloc(MFTP_HEADER_BUFFERSIZE);

	/* create request header */
	struct header_entry *req_headers = NULL;
	struct header_entry *item = malloc(sizeof(struct header_entry));
	strncpy(item->name, "filename", 512);
	strncpy(item->value, param, 512);
	HASH_ADD_STR(req_headers, name, item);

	create_header(header_buf,"qget",req_headers);
	printf("get=>%s\n",header_buf);
	if( fwrite(header_buf,strlen(header_buf),1,handle.out) < 0 )
	{
	    fprintf(stderr,"fwrite() failed\n");
		goto free_header_buf;
	}

	printf("GETTING!!\n",header_buf);
	int cmd = parse_command(handle.in);
	printf("cmd!! %d\n",cmd);
	if (cmd == MFTP_RES_FAIL)
	{
		goto free_header_buf;
	}
	struct header_entry *res_headers = parse_headers(handle.in);
	char *filename = get_header_value(req_headers,"filename");
	printf("filename_parsed: %s!!\n",filename);

	FILE *fp = file_open(filename, 1);
	if (fp == NULL)
	{
		printf("[GET] file_open() failed.");
		goto free_2;
	}


	//file_decompressto(handle.in,stdout);
	file_decompressto(handle.in,fp);
	fclose(fp);

free_2:
	
free_header_buf:
	free(header_buf);
}
void mftpc_put(const char *param)
{
	if (param == NULL)
	{
		return;
	}
	char *filename = param;

	FILE *fp = file_open(filename, 0);
	if (fp == NULL)
	{
		printf("[PUT] file_open() failed.");
		return;
	}

	char *basec = strdup(param);
	char *bname = basename(basec);
	printf("bname = %s\n",bname);

	char *header_buf = (char *)malloc(MFTP_HEADER_BUFFERSIZE);
	struct header_entry *headers = NULL;
	struct header_entry *item = malloc(sizeof(struct header_entry));
	strncpy(item->name, "filename", 512);
	strncpy(item->value, bname, 512);
	HASH_ADD_STR(headers, name, item);
	create_header(header_buf,"qput",headers);

	fwrite(header_buf,strlen(header_buf),1,handle.out);
	file_compressto(fp,handle.out);
	fclose(fp);

	free(header_buf); /* malloc */

	free(basec); /* strdup */
	return;
}

void mftpc_open(char *param)
{
	char *open_host = strdup(param);
	char *port = split(open_host,':');
	int open_port = atoi(port);

	mftpc_conn(open_host,open_port);
	printf("[OPEN] %s:%d\n",open_host,open_port);
	free(open_host); /* strdup */
}

/* interpret user's inputs */
char* lexer(char *cmd)
{
	if (cmd == NULL) return;
	int cur = 0;

	/* parsing command */
	while (!isalpha(cmd[0]))
	{
		cmd++;
	}
	
	do {
		cur++;
	} while (cmd[cur] != ' ');
	cmd[cur] = '\0';

	/* parsing parameter */
	return cmd + cur + 1;
}

int main( int argc, char *argv[] )
{
	if (argc == 3)
	{
		int port = atoi(argv[2]);
		mftpc_conn(argv[1],port);
	}
	else if (argc == 2)
	{
		mftpc_conn(argv[1],10000);
	}
	char prompt[128] = "mFTP> ";
	int history_no = 0;
	HIST_ENTRY *history = NULL;

	show_prompt(prompt);
	cwd_init();
	while ( (cmd = readline(prompt)) )
	{
		/* add to readline history */
		add_history(cmd);
		char *param = lexer(cmd);
		if (strcmp(cmd,"get")==0)
		{
			mftpc_get(param);
		}
		else if (strcmp(cmd,"put")==0)
		{
			mftpc_put(param);
		}
		else if (strcmp(cmd,"open")==0)
		{
			mftpc_open(param);
		}

		show_prompt(prompt);

		/* clear old readline history */
		if (++history_no > MAX_HISTORY_CNT)
		{
			history = remove_history(0);
			free(history);
		}
	}
	return 0;
}


