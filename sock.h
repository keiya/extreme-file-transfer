#include <sys/socket.h>	/* socket() */
int fdopen_sock( int, FILE **, FILE ** );
int tcp_acc_port( int, int );
int tcp_connect( char *, int );
int tcp_accept(int, struct sockaddr *, socklen_t *);
