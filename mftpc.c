/*
 * mftpc.c (XFT / Extreme File Transfer)
 * Keiya Chinen <s1011420@coins.tsukuba.ac.jp>
 */
#include <ctype.h>
#include <libgen.h> /* basename */
#include <netinet/in.h>	/* struct sockaddr_in */
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "cwd.h"
#include "file.h"
#include "ls.h"
#include "protocol.h"
#include "sock.h"
#include "timeutil.h"
#include "utility.h"

#define MAX_HISTORY_CNT 10
#define MEGA 1000000

struct mftpc_conn_handle {
	FILE *in;
	FILE *out;
};

struct mftpc_conn_handle handle;
int connected = 0;

struct cwd_ctx* cwd;

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

void print_stats(struct timespec *start, struct timespec *end, size_t netsz, size_t iosz)
{
	long long unsigned int diff = nanodiff(start,end);
	double sec = (double)diff / 1000000000.0;
	double netio = (double)netsz / (double)iosz * 100.0;
	printf("Transferred %d bytes (Extracted: %d bytes, Net/IO: %.2f %%) in %.3f sec\nNetwork Throughput: %.3f MB/s, IO Throughput: %.3f MB/s\n",
					(int)netsz,(int)iosz,netio,
					sec,
					netsz / MEGA / sec, iosz / MEGA / sec);
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
	struct timespec start, end;

	char *req_filename = strdup(param);
	char *header_buf = (char *)malloc(MFTP_HEADER_BUFFERSIZE);

	/* create request header */
	struct header_entry *req_headers = NULL;
	struct header_entry *item = malloc(sizeof(struct header_entry));
	strncpy(item->name, "filename", 512);
	strncpy(item->value, req_filename, 512);
	HASH_ADD_STR(req_headers, name, item);

	create_header(header_buf,"qget",req_headers);
	free_header(req_headers);
	fwrite(header_buf,strlen(header_buf),1,handle.out);


	/* == clock start == */
	current_utc_time(&start);

	int cmd = parse_command(handle.in);
	if (cmd == MFTP_FAIL)
	{
		goto free_1;
	}

	struct header_entry *res_headers = parse_headers(handle.in);

	/*char *save_filename = get_header_value(res_headers,"filename");*/
	char *save_filename = req_filename;

	if (res_headers != NULL)
	{
		free_header(res_headers);
	}

	FILE *fp = file_open(save_filename, 1);
	if (fp == NULL)
	{
		printf("[GET] file_open() failed.");
		goto free_2;
	}

	size_t net_recv = 0; size_t io_wrote = 0;
	file_decompressto(handle.in,fp,&net_recv,&io_wrote); /* receive, extract, write */
	ret = fclose(fp);
	current_utc_time(&end);
	/* == clock end == */
	print_stats(&start,&end,net_recv,io_wrote);

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

	struct timespec start, end;

	char *basec = strdup(param);
	char *dst_filename = basename(basec);

	char *header_buf = (char *)malloc(MFTP_HEADER_BUFFERSIZE);
	struct header_entry *headers = NULL;
	struct header_entry *item = malloc(sizeof(struct header_entry));
	strncpy(item->name, "filename", 512);
	strncpy(item->value, dst_filename, 512);
	HASH_ADD_STR(headers, name, item);
	create_header(header_buf,"qput",headers);
	free_header(headers);

	/* == clock start == */
	current_utc_time(&start);
	fwrite(header_buf,strlen(header_buf),1,handle.out);

	size_t io_read = 0; size_t net_sent = 0;
	file_compressto(fp,handle.out,&io_read,&net_sent);
	ret = fclose(fp);
	current_utc_time(&end);
	/* == clock end == */
	print_stats(&start,&end,net_sent,io_read);

	/* receive response from server? */

	free(header_buf); /* malloc */

	free(basec); /* strdup */
free_1:
	free(src_filename);
	return ret;
}

void mftpc_dir(char *param)
{
	char target[MFTP_HEADER_BUFFERSIZE_HALF];
	void *crl = pinit();
	if (crl == NULL)
	{
		perror("curl");
		return;
	}
	char *encoded;
	if (param == NULL || param[0] == '\0')
	{
		strcpy(target,"./");
	}
	else
	{
		encoded = pencode(crl,target);
		strncpy(target,param,MFTP_HEADER_BUFFERSIZE_HALF);
		pfree(encoded);
	}

	fprintf(handle.out,"qdir\r\ndirname:%s\r\n\r\n",target);

	int cmd = parse_command(handle.in);
	if (cmd == MFTP_FAIL)
	{
		goto free_1;
	}

	struct header_entry *res_headers = parse_headers(handle.in);

	char *listing_dir = get_header_value(res_headers,"dirname");
	free_header(res_headers); /* parse_headers */

	printf("[DIR]%s\n",listing_dir);
	free(listing_dir);

	char line[1024];
	char *decoded;
	while ( fgets(line,1024,handle.in) != NULL )
	{
		if (strstr(line,"\r\n") == line)
			break;
		char *filename = strrchr(line,'\t');
		if (filename == NULL) continue;
		chomp(filename);
		decoded = pdecode(crl,filename);
		if (decoded != NULL)
		{
			//fputs(line,stdout);
			fwrite(line,1,filename-line,stdout); // write out exclude a filename
			fputs(decoded,stdout);
		}
		printf("\n");
		pfree(decoded);
	}

free_1:
	pclean(crl);
}

int mftpc_cd(char *param)
{
	int status = 0;
	if (param == NULL || param[0] == '\0')
	{
		return -1;
	}
	void *crl = pinit();
	if (crl == NULL)
	{
		perror("curl");
		return -1;
	}
	char *encoded;
	encoded = pencode(crl,param);
	fprintf(handle.out,"qcd\r\ndirname:%s\r\n\r\n",encoded);
	pfree(encoded);

	int cmd = parse_command(handle.in);
	if (cmd == MFTP_FAIL)
	{
		status = -1;
		goto free_1;
	}

	struct header_entry *res_headers = parse_headers(handle.in);

	char *listing_dir = get_header_value(res_headers,"dirname");
	printf("resheaders:%p\n",res_headers);
	free_header(res_headers); /* parse_headers */
	printf("[DIR]%s\n",listing_dir);
	free(listing_dir);

free_1:
	pclean(crl);
	return status;
}

void mftpc_lcd(char *param)
{
	if ( ! cwd_chdir(cwd,param,1))
	{
		perror("cd");
	}
}

void mftpc_lls(char *param)
{
	struct lsent lse;
	if (param == NULL)
		ls(".",&lse); /* current dir */
	else
		ls(param,&lse);
	display(&lse,stdout,0);
	free_lse(&lse);
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
	int len = strlen(cmd);
	if (cmd == NULL || len <= 0) return NULL;
	int cur = 0;

	/* shell command */
	if (cmd[0] == '!')
		cmd++;

	/* parsing command */
	while (!isalpha(cmd[0]))
	{
		cmd++;
	}
	
	do {
		cur++;
	}
	while (cmd[cur] != '\0' && cmd[cur] != ' ');
	cmd[cur] = '\0';
	if (len <= cur + 1)
		return NULL;

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
	cwd = cwd_init();
	char *input;
	while ( (input = readline(prompt)) )
	{
		/* add to readline history */
		add_history(input);
		char *cmd = strdup(input);
		char *param = lexer(cmd);
		int status = 0;
		if (strcmp(cmd,"get")==0) /* fetch data from remote */
		{
			if (check_connected())
			{
				status = mftpc_get(param);
			}
		}
		else if (strcmp(cmd,"put")==0) /* send data to remote */
		{
			if (check_connected())
			{
				status = mftpc_put(param);
			}
		}
		else if (strcmp(cmd,"open")==0) /* open a connection */
		{
			mftpc_open(param);
		}
		else if (strcmp(cmd,"cd")==0) /* remote cd */
		{
			status = mftpc_cd(param);
		}
		else if (strcmp(cmd,"lcd")==0) /* local cd */
		{
			mftpc_lcd(param);
		}
		else if (strcmp(cmd,"ls")==0 || strcmp(cmd,"dir")==0) /* remote ls */
		{
			mftpc_dir(param);
		}
		else if (strcmp(cmd,"!ls")==0 || strcmp(cmd,"!dir")==0) /* local ls */
		{
			mftpc_lls(param);
		}
		if (status)
		{
			printf("error %d\n",status);
		}

		show_prompt(prompt);

		/* clear old readline history */
		if (++history_no > MAX_HISTORY_CNT)
		{
			history = remove_history(0);
			free(history);
		}
		free(cmd); /* strdup() */
	}
	return 0;
}


