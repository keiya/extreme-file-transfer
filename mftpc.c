/*
 * mftpc.c (XFT / Extreme File Transfer)
 * Keiya Chinen <s1011420@coins.tsukuba.ac.jp>
 */
#include <libgen.h> /* basename */
#include <netinet/in.h>	/* struct sockaddr_in */
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cwd.h"
#include "file.h"
#include "protocol.h"
#include "sock.h"
#include "utility.h"

#define MAX_HISTORY_CNT 10

char *cmd;

struct mftpc_conn_handle {
	FILE *in;
	FILE *out;
};

struct mftpc_conn_handle handle;
int connected = 0;

void sigint_action(int signum, siginfo_t *info, void *ctx)
{
#ifdef DEBUG
	printf("sigint_handler(%d): signo(%d) code(0x%x)\n",
		signum, info->si_signo, info->si_code);
#endif
}
void sigpipe_action(int signum, siginfo_t *info, void *ctx)
{
	connected = 0;
	/*
	printf("sigpipe_handler(%d): signo(%d) code(0x%x)\n",
		signum, info->si_signo, info->si_code);
	*/
}

void set_sighandler()
{
	struct sigaction sa_sigint, sa_sigpipe;
	
	memset(&sa_sigint, 0, sizeof(sa_sigint));
	sa_sigint.sa_sigaction = sigint_action;
	sa_sigint.sa_flags = SA_RESTART | SA_SIGINFO;

	memset(&sa_sigpipe, 0, sizeof(sa_sigpipe));
	sa_sigpipe.sa_sigaction = sigpipe_action;
	sa_sigpipe.sa_flags = SA_RESTART | SA_SIGINFO;
	
	if (sigaction(SIGINT, &sa_sigint, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}
	else if (sigaction(SIGPIPE, &sa_sigpipe, NULL) < 0) {
		printf("sigpipe\n");
	}

}

void show_prompt(char *buf)
{
	char cwd_str[128];
	if (getcwd(cwd_str,128) != NULL)
		snprintf(buf,128,"[%s](mFTP)> ",cwd_str);
	else
		snprintf(buf,128,"mFTP> ");
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
	connected = 1;
	return 1;
}
int mftpc_get(const char *param)
{
	int ret = -1;
	if (param == NULL)
	{
		return -1;
	}
	char *req_filename = strdup(param);
	char *header_buf = (char *)malloc(MFTP_HEADER_BUFFERSIZE);

	/* create request header */
	struct header_entry *req_headers = NULL;
	struct header_entry *item = malloc(sizeof(struct header_entry));
	strncpy(item->name, "filename", 512);
	strncpy(item->value, req_filename, 512);
	HASH_ADD_STR(req_headers, name, item);

	create_header(header_buf,"qget",req_headers);
	printf("get=>%s\n",header_buf);
	if( fwrite(header_buf,strlen(header_buf),1,handle.out) < 0 )
	{
	    fprintf(stderr,"fwrite() failed\n");
		goto free_1;
	}

	printf("GETTING!!\n");
	int cmd = parse_command(handle.in);
	printf("cmd!! %d\n",cmd);
	if (cmd == MFTP_FAIL)
	{
		goto free_1;
	}
	struct header_entry *res_headers = parse_headers(handle.in);
	char *res_filename = get_header_value(res_headers,"filename");
	printf("filename_parsed: %s!!\n",res_filename);

	FILE *fp = file_open(res_filename, 1);
	if (fp == NULL)
	{
		printf("[GET] file_open() failed.");
		goto free_2;
	}


	//file_decompressto(handle.in,stdout);
	file_decompressto(handle.in,fp);
	ret = fclose(fp);

free_2:
	
free_1:
	free(header_buf);
	free(req_filename);
	return ret;
}

int mftpc_put(const char *param)
{
	int ret = -1;
	if (param == NULL)
	{
		return -1;
	}
	char *src_filename = strdup(param);

	FILE *fp = file_open(src_filename, 0);
	if (fp == NULL)
	{
		printf("[PUT] file_open() failed.");
		goto free_1;
	}

	char *basec = strdup(param);
	char *dst_filename = basename(basec);
	printf("bname = %s\n",dst_filename);

	char *header_buf = (char *)malloc(MFTP_HEADER_BUFFERSIZE);
	struct header_entry *headers = NULL;
	struct header_entry *item = malloc(sizeof(struct header_entry));
	strncpy(item->name, "filename", 512);
	strncpy(item->value, dst_filename, 512);
	HASH_ADD_STR(headers, name, item);
	create_header(header_buf,"qput",headers);
	printf("HEADER=[%s]\n",header_buf);

	fwrite(header_buf,strlen(header_buf),1,handle.out);
	file_compressto(fp,handle.out);
	ret = fclose(fp);

	free(header_buf); /* malloc */

	free(basec); /* strdup */
free_1:
	free(src_filename);
	return ret;
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
	if (cmd == NULL) return NULL;
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

int check_connected()
{
	if (connected == 1) return 1;
	printf("not connected.");
	return 0;
}

int main( int argc, char *argv[] )
{
	set_sighandler();
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
			if (check_connected())
			{
				int res = mftpc_get(param);
				if (res == -1)
					printf("failed\n");
			}
		}
		else if (strcmp(cmd,"put")==0)
		{
			if (check_connected())
			{
				int res = mftpc_put(param);
				if (res == -1)
					printf("failed\n");
			}
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


