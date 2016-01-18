/*
 * cwd.c (current working directory utility)
 * Keiya Chinen <s1011420@coins.tsukuba.ac.jp>
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>

char root_path[PATH_MAX+1];
char cwd_path[PATH_MAX+1];

/*
int main(void)
{
	cwd_init();
	cwd_set_root("/home/dux/");
	while (1)
	{
		char str[PATH_MAX+1];
		fgets(str,PATH_MAX,stdin);
		chomp(str);
		cwd_chdir(str);
		printf("%s\n",cwd_path);
	}
	return 0;
}
*/

void cwd_init()
{
	root_path[0] = '/';
	root_path[1] = '\0';
	cwd_path[0] = '/';
	cwd_path[1] = '\0';
}

int cwd_is_statable(const char* path)
{
	struct stat stat_buf;
	if (stat(path, &stat_buf) == 0)
		return 1;
	else return 0;
}

void cwd_get_path(char *buf)
{
	size_t buf_size;
	buf_size = strlen(buf);
	strncpy(buf,cwd_path,buf_size);
	buf[buf_size] = '\0';
}

// document-root
int cwd_set_root(const char* setrootpath)
{
	size_t setrootpath_size, cpy_size;
	setrootpath_size = strlen(setrootpath);
	// equivalent: min(rootpath_size,PATH_MAX)
	cpy_size = setrootpath_size < PATH_MAX ? setrootpath_size : PATH_MAX;
	if (! realpath(setrootpath,root_path))
		return 0;
	strncpy(cwd_path,root_path,PATH_MAX);
	return 1;
}

int cwd_chdir(const char* chdirpath)
{
	char path_buf[PATH_MAX+1];
	char fullpath_buf[PATH_MAX+1];
	char resolved_path[PATH_MAX+1];

	strncpy(fullpath_buf,cwd_path,PATH_MAX);

	cwd_get_path(cwd_path);
	if (strchr(chdirpath,'/') == chdirpath)
	{ // slash(/) --> document-root(dir_set_root)
		strncpy(path_buf,root_path,PATH_MAX);
		strncat(path_buf,chdirpath,PATH_MAX);
#ifdef DEBUG
		printf("slash %s\n",path_buf);
#endif
		strncpy(fullpath_buf,path_buf,PATH_MAX);
	}
	else if (0 == strcmp(chdirpath,"."))
	{
		strncpy(path_buf,"/.",PATH_MAX);
#ifdef DEBUG
		printf(". %s\n",path_buf);
#endif
		strncat(fullpath_buf,path_buf,PATH_MAX);
	}
	else
	{
		strncpy(path_buf,"/",PATH_MAX);
		strncat(path_buf,chdirpath,PATH_MAX);
#ifdef DEBUG
		printf("noslash %s\n",path_buf);
#endif
		strncat(fullpath_buf,path_buf,PATH_MAX);
	}


#ifdef DEBUG
	printf("pb>%s<\n",path_buf);
	printf("fpb>%s<\n",fullpath_buf);
#endif
	if (! realpath(fullpath_buf,resolved_path))
		return 0;
#ifdef DEBUG
	printf("resolved:%s root_path:%s\n",resolved_path,root_path);
#endif
	if (NULL == strstr(resolved_path,root_path))
	{ // directory climbing (directory traversal)
		return 0;
	}
	else
	{
	}
	strncpy(cwd_path,resolved_path,PATH_MAX);
#ifdef DEBUG
	printf("cwd>%s<\n",cwd_path);
#endif
	if (chdir(cwd_path) != 0)
	{
		return 0;
	}
	return 1;
}
