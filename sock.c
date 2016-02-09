#include <netdb.h>	/* getnameinfo() */
#include <sys/types.h>	/* socket(), wait4() */
#include <sys/socket.h>	/* socket() */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define PORTNO_BUFSIZE 30
int
fdopen_sock( int sock, FILE **inp, FILE **outp )
{
	int sock2 ;
	if( (sock2=dup(sock)) < 0 )
	{
	    return( -1 );
	}
	if( (*inp = fdopen( sock2, "r" )) == NULL )
	{
	    close( sock2 );
	    return( -1 );
	}
	if( (*outp = fdopen( sock, "w" )) == NULL )
	{
	    fclose( *inp );
	    *inp = 0 ;
	    return( -1 );
	}
//	setvbuf(*outp, (char *)NULL, _IONBF, 0);
	return( 0 );
}

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

int tcp_accept(int acc, struct sockaddr *addr, socklen_t *addrlen)
{
	return accept( acc,addr,addrlen );
}
