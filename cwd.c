/*
 * cwd.c (current working directory utility)
 * Keiya Chinen <s1011420@coins.tsukuba.ac.jp>
 */
#include <libgen.h> /* basename */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include "cwd.h"

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

struct cwd_ctx* cwd_init()
{
	struct cwd_ctx *ctx = (struct cwd_ctx *)malloc(sizeof(struct cwd_ctx));
	ctx->root_path[0] = '/';
	ctx->root_path[1] = '\0';
	ctx->cwd_path[0] = '/';
	ctx->cwd_path[1] = '\0';
	return ctx;
}

int cwd_is_statable(const char* path)
{
	struct stat stat_buf;
	if (stat(path, &stat_buf) == 0)
		return 1;
	else return 0;
}

void cwd_get_path(struct cwd_ctx *ctx,char *buf)
{
	strncpy(buf,ctx->cwd_path,PATH_MAX);
}

void cwd_realpath(struct cwd_ctx *ctx,const char *dirname,char *path)
{
	if (strchr(dirname,'/') == dirname) /* absolute path */
	{
		realpath(dirname,path);
	}
	else /*relative path*/
	{
		char tmp[PATH_MAX];
		char cwdpath[PATH_MAX];
		cwd_get_path(ctx,cwdpath);
		snprintf(tmp,PATH_MAX,"%s/%s",cwdpath,dirname);	
		realpath(tmp,path);
	}
}

// document-root
int cwd_set_root(struct cwd_ctx *ctx, const char* setrootpath)
{
	size_t setrootpath_size, cpy_size;
	setrootpath_size = strlen(setrootpath);
	// equivalent: min(rootpath_size,PATH_MAX)
	setrootpath_size < PATH_MAX ? (cpy_size = setrootpath_size) : (cpy_size = PATH_MAX);
	if (! realpath(setrootpath,ctx->root_path))
		return 0;
	strncpy(ctx->cwd_path,ctx->root_path,PATH_MAX);
	return 1;
}

int cwd_chdir(struct cwd_ctx *ctx, const char* chdirpath, int change)
{
	char path_buf[PATH_MAX+1];
	char fullpath_buf[PATH_MAX+1];
	char resolved_path[PATH_MAX+1];

	strncpy(fullpath_buf,ctx->cwd_path,PATH_MAX);

	if (strchr(chdirpath,'/') == chdirpath)
	{ // slash(/) --> document-root(dir_set_root)
		strncpy(path_buf,ctx->root_path,PATH_MAX);
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
	printf("resolved:%s root_path:%s\n",resolved_path,ctx->root_path);
#endif
	if (NULL == strstr(resolved_path,ctx->root_path))
	{ // directory climbing (directory traversal)
		return 0;
	}
	else
	{
	}
	strncpy(ctx->cwd_path,resolved_path,PATH_MAX);
#ifdef DEBUG
	printf("cwd>%s<\n",ctx->cwd_path);
#endif
	if (change)
	{
		if (chdir(ctx->cwd_path) != 0)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else {
		return 1;
	}
}
