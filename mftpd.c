/*
 * mftpd.c (XFT / Extreme File Transfer)
 * Keiya Chinen <s1011420@coins.tsukuba.ac.jp>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* strlen() */
#include <sys/types.h>	/* socket(), wait4() */
#include <sys/socket.h>	/* socket() */
#include <pthread.h>	/* pthread_create */
#include <netdb.h>	/* getnameinfo() */
#include <stdarg.h>
#include <unistd.h>
#include "mftpd.h"
#include "utility.h"
#include "file.h"
#include "protocol.h"
#include <libgen.h> /* basename */


/* Garbage collector context */
void *gc_ctx;

struct mftpd_thread_arg {
	int com;
	FILE *in;
	FILE *out;
};

void* mftpd_conn_thread(struct mftpd_thread_arg *);
void debug_print(const char *, ...);

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
#include <stdio.h>
	{
		if( (com = accept( acc,0,0 )) < 0 )
		{
			perror("accept");
			exit( -1 );
		}
		arg = malloc( sizeof(struct mftpd_thread_arg) );
		if( arg == NULL )
		{
			perror("malloc()");
			exit( -1 );
		}
		arg->com = com;
		if( pthread_create( &worker, NULL, (void *)mftpd_conn_thread, (void *)arg)
				!= 0 )
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
	//FILE *in, *out ;
	if( fdopen_sock(conn_arg->com,&conn_arg->in,&conn_arg->out) < 0 )
	{
		fprintf(stderr,"fdopen()\n");
		exit( 1 );
	}

#ifdef DEBUG
	debug_print("socket has been opened");
#endif
	/*
	struct mftpd_thread_arg *trans_arg;
	trans_arg = malloc( sizeof(struct mftpd_thread_arg) );
	if( trans_arg == NULL )
	{
		perror("malloc()");
		exit( -1 );
	}
	trans_arg->in = 
	*/
	while ( 1 )
	{
		mftpd_do_command(conn_arg);
	}
	fclose( conn_arg->in );
	fclose( conn_arg->out );

	free( conn_arg );
	return( NULL );
}

void mftpd_get_reply(const char *filename,FILE *out)
{
	printf("[get]file:%s\n",filename);
	if (filename == NULL)
	{
		return;
	}
	char *basec = strdup(filename);
	FILE* fp = file_open(filename, 0);
	if (fp != NULL)
	{
		printf("[get]fo:%s\n",filename);
		char *bname = basename(basec);
		printf("bname = %s\n",bname);

		char *header_buf = (char *)malloc(MFTP_HEADER_BUFFERSIZE);
		struct header_entry *headers = NULL;
		struct header_entry *item = malloc(sizeof(struct header_entry));
		strncpy(item->name, "filename", 512);
		strncpy(item->value, bname, 512);
		HASH_ADD_STR(headers, name, item);
		create_header(header_buf,"s20",headers);
	
		printf("[GET][s20]%s\n",filename);
		fwrite(header_buf,strlen(header_buf),1,out);
		file_compressto(fp,out);
		fclose(fp);
	}
	else
	{
		printf("[GET][s40]%s\n",filename);
		fprintf(out,"s40\r\n");
	}

	free(basec); /* strdup */
}

void mftpd_put_receive(const char *filename,FILE *in)
{
	if (filename == NULL)
	{
		return;
	}
	FILE *fp = file_open(filename, 1);
	if (fp == NULL)
	{
		printf("[PUT] file_open() failed.");
		return;
	}

	file_decompressto(in,fp);
	fclose(fp);
}

int
//mftpd_do_command(char* cmd, struct mftpd_thread_arg *conn_arg)
mftpd_do_command(struct mftpd_thread_arg *conn_arg)
{
	pthread_t worker;
	/* parse headers */
	int cmd = parse_command(conn_arg->in);
	struct header_entry *headers = parse_headers(conn_arg->in);

	if (cmd == MFTP_CMD_GET)
	{
		char *filename = get_header_value(headers,"filename");
		mftpd_get_reply(filename,conn_arg->out);
	}
	else if (cmd == MFTP_CMD_PUT)
	{
		char *filename = get_header_value(headers,"filename");
		mftpd_put_receive(filename,conn_arg->in);
	}
	else if (cmd == MFTP_CMD_CD)
	{
			
	}
	else if (cmd == MFTP_CMD_DIR)
	{
	
	}
	return 0;
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

#define PORTNO_BUFSIZE 30

int
tcp_acc_port( int portno, int ip_version )
{
	struct addrinfo hints, *ai;
	char portno_str[PORTNO_BUFSIZE];
	int err, s, on, pf;

	switch( ip_version )
	{
		case 4:
			pf = PF_INET;
			break;
		case 6:
			pf = PF_INET6;
			break;
		default:
			fprintf(stderr,"bad IP version: %d.  4 or 6 is allowed.\n",
					ip_version );
			goto error0;
	}
	snprintf( portno_str,sizeof(portno_str),"%d",portno );
	memset( &hints, 0, sizeof(hints) );
	ai = NULL;
	hints.ai_family   = pf ;
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM ;
	if( (err = getaddrinfo( NULL, portno_str, &hints, &ai )) )
	{
		fprintf(stderr,"bad portno %d? (%s)\n",portno,
				gai_strerror(err) );
		goto error0;
	}
	if( (s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0 )
	{
		perror("socket");
		goto error1;
	}

#ifdef	IPV6_V6ONLY
	if( ai->ai_family == PF_INET6 )
	{
		on = 1;
		if( setsockopt(s,IPPROTO_IPV6, IPV6_V6ONLY,&on,sizeof(on)) < 0 )
		{
			perror("setsockopt(,,IPV6_V6ONLY)");
			goto error1;
		}
	}
#endif	/*IPV6_V6ONLY*/

	if( bind(s,ai->ai_addr,ai->ai_addrlen) < 0 )
	{
		perror("bind");
		fprintf(stderr,"port number %d can be already used. wait a moment or kill another program.\n", portno );
		goto error2;
	}
	on = 1;
	if( setsockopt( s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) < 0 )
	{
		perror("setsockopt(,,SO_REUSEADDR)");
		goto error2;
	}
	if( listen( s, 5 ) < 0 )
	{
		perror("listen");
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

