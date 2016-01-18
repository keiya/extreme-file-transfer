#include <string.h>
#include <stdio.h>
char* chomp( char *str )
{
    int len ;
	len = strlen( str );
	if( len>=2 && str[len-2] == '\r' && str[len-1] == '\n' )
	{
	    str[len-2] = str[len-1] = 0;
	}
	else if( len >= 1 && (str[len-1] == '\r' || str[len-1] == '\n') )
	{
	    str[len-1] = 0;
	}
	return( str );
}


char* split(char *str, int delim)
{
	char *splitptr = NULL;
	if (NULL != (splitptr = strchr(str,delim)))
	{
		*splitptr = '\0';
		++splitptr;
	}
	return splitptr;
}


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
	setvbuf(*outp, (char *)NULL, _IONBF, 0);
	return( 0 );
}

