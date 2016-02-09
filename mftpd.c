/*
 * mftpd.c (XFT / Extreme File Transfer)
 * Keiya Chinen <s1011420@coins.tsukuba.ac.jp>
 */
#include <libgen.h> /* basename */
#include <limits.h>
#include <pthread.h>	/* pthread_create */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* strlen() */
#include <time.h>
#include <unistd.h>
#include "file.h"
#include "ls.h"
#include "protocol.h"
#include "sock.h"
#include "utility.h"
#include "cwd.h"


struct mftpd_thread_arg {
	int com;
	FILE *in;
	FILE *out;
	struct cwd_ctx *cwd;
};

void debug_print(const char *, ...);
void mftpd_listen( int, int );
int mftpd_do_command(struct mftpd_thread_arg *);
void* mftpd_conn_thread( struct mftpd_thread_arg * );
void mftpd_get_reply(const char *,FILE *,struct cwd_ctx *cwd);
void mftpd_put_receive(const char *,FILE *,struct cwd_ctx *cwd);

void debug_print(const char *format, ...)
{
	pthread_t tid = pthread_self();
	va_list args;
	va_start(args, format);

	printf("[%d/%d]",getpid(),(int)tid);
	vprintf(format, args);
	printf("\n");

	va_end(args);
}

void printl(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	time_t rawtime;
	struct tm * timeinfo;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	printf ("%s:", asctime (timeinfo) );
	vprintf(format, args);
	printf("\n");

	va_end(args);
}

void
mftpd_listen( int portno, int ip_version )
{
	int acc,com;
	pthread_t worker ;
	struct mftpd_thread_arg *arg;
	acc = tcp_acc_port( portno, ip_version );
	if( acc < 0 ) {
		perror("tcp_acc_port");
		exit( -1 );
	}
#ifdef DEBUG
	debug_print("Listening on %d (fd==%d)",portno,acc);
#endif
	while( 1 )
	{
		if( (com = tcp_accept( acc,0,0 )) < 0 )
		{
			perror("accept");
			exit( -1 );
		}
#ifdef DEBUG
	debug_print("accept (%d)",com);
#endif
		arg = malloc( sizeof(struct mftpd_thread_arg) );
 		if( arg == NULL )
 		{
 			perror("malloc()");
 			exit( -1 );
 		}
		arg->com = com;
		if( pthread_create( &worker, NULL, (void *)mftpd_conn_thread, (void *)arg) != 0)
		{
			perror("pthread_create()");
			exit( 1 );
		}
#ifdef DEBUG
		debug_print("ConnThread has been created");
#endif
		pthread_detach( worker );
	}
}

// connection thread handles each connection
void*
mftpd_conn_thread( struct mftpd_thread_arg *conn_arg )
{
	if( fdopen_sock(conn_arg->com,&conn_arg->in,&conn_arg->out) < 0 )
	{
		fprintf(stderr,"fdopen()\n");
		exit( 1 );
	}

#ifdef DEBUG
	debug_print("socket has been opened");
#endif

	conn_arg->cwd = cwd_init();
	while ( mftpd_do_command(conn_arg) )
		;
	fclose( conn_arg->in );
	fclose( conn_arg->out );
	free(conn_arg);

#ifdef DEBUG
	debug_print("socket has been closed");
#endif

	return( NULL );
}

void mftpd_get_reply(const char *filename,FILE *out,struct cwd_ctx *cwd)
{
	if (filename == NULL)
	{
		return;
	}
	char path[PATH_MAX];
	cwd_realpath(cwd,filename,path);
	char *basec = strdup(path);
	FILE* fp = file_open(path, 0);
	if (fp != NULL)
	{
		char *bname = basename(basec);

		char *header_buf = (char *)malloc(MFTP_HEADER_BUFFERSIZE);
		struct header_entry *headers = NULL;
		struct header_entry *item = malloc(sizeof(struct header_entry));
		strncpy(item->name, "filename", 512);
		strncpy(item->value, bname, 512);
		HASH_ADD_STR(headers, name, item);
		create_header(header_buf,"s20",headers);
		free_header(headers);
	
		printl("[GET][s20]%s\n",path);
		fwrite(header_buf,strlen(header_buf),1,out);
		fflush(out);
		free(header_buf); /* malloc */

		size_t io_read, net_sent = 0;
		file_compressto(fp,out,&io_read,&net_sent);
		fflush(out);
		fclose(fp);
	}
	else
	{
		printl("[GET][s40]%s\n",path);
		fprintf(out,"s40\r\n");
		fflush(out);
	}

	free(basec); /* strdup */
}

void mftpd_put_receive(const char *filename,FILE *in,struct cwd_ctx *cwd)
{
	if (filename == NULL)
	{
		return;
	}
	char path[PATH_MAX];
	cwd_realpath(cwd,filename,path);
	FILE *fp = file_open(path, 1);
	if (fp == NULL)
	{
		printf("[PUT] file_open() failed.");
		return;
	}

	printl("[PUT]%s\n",path);

	size_t net_recv, io_wrote = 0;
	file_decompressto(in,fp, &net_recv, &io_wrote);

	/* return response to client? */

	fclose(fp);
}

void mftpd_cd(const char *dirname,FILE *out,struct cwd_ctx* cwd)
{
	if ( ! cwd_chdir(cwd,dirname,0))
	{
		perror("cd");
	}
	char buf[PATH_MAX];
	cwd_get_path(cwd,buf);
	fprintf(out,"s20\r\ndirname:%s\r\n\r\n",buf);
	fflush(out);
}

void mftpd_dir(const char *dirname,FILE *out, struct cwd_ctx* cwd)
{
	char path[PATH_MAX];
	cwd_realpath(cwd,dirname,path);

	fprintf(out,"s20\r\ndirname:%s\r\n\r\n",path);
	fflush(out);

	struct lsent lse;
	ls(path,&lse);
	display(&lse,out,1);
	free_lse(&lse);

	fprintf(out,"\r\n");
	fflush(out);
}

int
mftpd_do_command(struct mftpd_thread_arg *conn_arg)
{
	/* parse headers */
	int cmd;

	/* return 0 if EOF */
	if ( ! (cmd = parse_command(conn_arg->in)) )
	{
		return 0;
	}

	struct header_entry *headers = parse_headers(conn_arg->in);

	void *crl = pinit();
	if (crl == NULL) return 0;
	if (cmd == MFTP_CMD_GET)
	{
		char *filename = get_header_value(headers,"filename");
		char *decoded = pdecode(crl,filename);
		mftpd_get_reply(decoded,conn_arg->out,conn_arg->cwd);
		free(filename); /* get_header_value */
		pfree(decoded);
	}
	else if (cmd == MFTP_CMD_PUT)
	{
		char *filename = get_header_value(headers,"filename");
		char *decoded = pdecode(crl,filename);
		mftpd_put_receive(decoded,conn_arg->in,conn_arg->cwd);
		free(filename); /* get_header_value */
		pfree(decoded);
	}
	else if (cmd == MFTP_CMD_CD)
	{
		char *dirname = get_header_value(headers,"dirname");
		char *decoded = pdecode(crl,dirname);
		mftpd_cd(decoded,conn_arg->out,conn_arg->cwd);
		free(dirname); /* get_header_value */
		pfree(decoded);
	}
	else if (cmd == MFTP_CMD_DIR)
	{
		char *dirname = get_header_value(headers,"dirname");
		char *decoded = pdecode(crl,dirname);
		mftpd_dir(decoded,conn_arg->out,conn_arg->cwd);
		free(dirname); /* get_header_value */
		pfree(decoded);
	}
	else
	{
		fprintf(conn_arg->out,"s40\r\n\r\n");
		fflush(conn_arg->out);
	}
	pclean(crl);
	return 1;
}


int main( int argc, char *argv[] )
{
	int portno, ip_version;
	if( !(argc == 2 || argc==3) ) {
		fprintf(stderr,"Usage: %s portno {ipversion}\n",argv[0] );
		exit( 1 );
	}
	portno = strtol( argv[1],0,10 );
	if( argc == 3 )
		ip_version = strtol( argv[2],0,10 );
	else
		ip_version = 4; /* IPv4 by default */
	mftpd_listen( portno,ip_version );
	return 0;
}


