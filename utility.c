#include <string.h>
#include <stdio.h>
#include <curl/curl.h>

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

void* pinit()
{
    return (void*)curl_easy_init();
}

char* pencode(void *crl, const char *str)
{
    return curl_easy_escape((CURL*)crl,str,0);
}

char* pdecode(void *crl, const char *str)
{
    return curl_easy_unescape((CURL*)crl,str,0,0);
}

void pfree(char *p)
{
    curl_free(p);
}

void pclean(void *crl)
{
	curl_easy_cleanup((CURL*)crl);
}
