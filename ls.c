/*
 * ls.c
 * Keiya Chinen <s1011420@coins.tsukuba.ac.jp>
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <time.h>
#include <libgen.h>
#include "ls.h"
#include "utility.h"

struct filemeta {
	char filename[NAME_MAX+1];
	char realpath[PATH_MAX+1];
	mode_t mode;
	nlink_t nlink;
	uid_t uid;
	gid_t gid;
	off_t size;
	blkcnt_t nblock;
	time_t mtime;
	time_t ctime;
};

int statfile(struct filemeta *file, char *path)
{
	struct stat statbuf;
	if (stat(path, &statbuf) != 0)
	{
		fprintf(stderr,"%s: stat failed\n",path);
		return 0;
	}
	strncpy(file->filename,path,NAME_MAX);
	// realpath() makes a lot of getcwd() calls...
	if (!realpath(path,file->realpath)) return 0;
	file->mode = statbuf.st_mode;
	file->nlink = statbuf.st_nlink;
	file->uid = statbuf.st_uid;
	file->gid = statbuf.st_gid;
	file->size = statbuf.st_size;
	file->mtime = statbuf.st_mtim.tv_sec;
	file->ctime = statbuf.st_ctim.tv_sec;
	file->nblock = statbuf.st_blocks;
	return 1;
}

int ls_dir(char *dir, struct lsent *lse)
{
	struct dirent *entry;
	DIR *dp;
	int i;

	if(( dp = opendir(dir) ) == NULL )
	{
		perror("opendir");
		exit( EXIT_FAILURE );
	}

	int file_count;
        file_count = 0;
	while ((entry = readdir(dp)) != NULL)
	{
		if (entry->d_type == DT_REG ||
			entry->d_type == DT_DIR ||
			entry->d_type == DT_LNK)
		{
			file_count++;
		}
	}

	struct filemeta **files;
	files = (struct filemeta **)malloc( sizeof(struct filemeta *) * file_count );
	if( files == NULL )
	{
		perror("malloc()");
		exit( -1 );
	}
	for (i=0; i < file_count; i++)
	{
		files[i] = (struct filemeta *)malloc(sizeof(struct filemeta));
		if (files[i] == NULL)
		{
			perror("malloc()");
			exit( -1 );
		}
	}

	rewinddir(dp);
	i = 0;
	while((entry = readdir(dp)) != NULL)
	{
		if ( ! (entry->d_type == DT_REG ||
			entry->d_type == DT_DIR ||
			entry->d_type == DT_LNK) )
		{
			continue;
		}
		if (i > file_count) break;

		size_t size = strlen(dir) + strlen(entry->d_name) + 2;
		char *fullpath = malloc(size);
		if (fullpath == NULL) { ++i; continue; }
		snprintf(fullpath,size, "%s/%s", dir, entry->d_name);
		statfile(files[i],fullpath);
		free(fullpath);
#ifdef DEBUG
	printf("ls_dir(%d)>%s\n",i,entry->d_name);
#endif
		++i;
	}
	closedir(dp);
	lse->files_p = files;
	lse->files_cnt = file_count;
	return 1;
}

void ls_file(char *filename, struct lsent *lse)
{
	struct filemeta **files;
	files = malloc( sizeof(struct filemeta *));
	if( files == NULL )
	{
		perror("malloc()");
		exit( -1 );
	}
	files[0] = malloc(sizeof(struct filemeta));
	if (files[0] == NULL)
	{
		perror("malloc()");
		exit( -1 );
	}
	statfile(files[0],filename);
	lse->files_p = files;
	lse->files_cnt = 1;
}

void free_lse(struct lsent *lse)
{
	int i;
	for (i=0; i<lse->files_cnt; ++i)
	{
		free(lse->files_p[i]);
	}
	free(lse->files_p);
}

int ls(char *path, struct lsent *lse)
{

	struct stat statbuf;
	if (stat(path, &statbuf) == 0)
	{
		//if( statbuf.st_mode & S_IFDIR )
		if(S_ISDIR(statbuf.st_mode))
		{
#ifdef DEBUG
			printf("target %s is dir\n",path);
#endif
			ls_dir(path,lse);
		}
		if(S_ISREG(statbuf.st_mode))
		//else if( statbuf.st_mode & S_IFREG )
		{
#ifdef DEBUG
			printf("target %s is file\n",path);
#endif
			ls_file(path,lse);
		}
		else{}
	}
	else
	{
		printf("%s: stat failed\n",path);
		return 0;
	}

	return 1;
}

void display(struct lsent *lse, FILE* out, int encode)
{
#ifdef DEBUG
	printf("%d\n",lse->files_cnt);
	printf("%p\n",lse->files_p);
#endif
	char outstr[200];
	//char modestr[11];
	int i;
	void *crl = pinit();
	if (crl == NULL) return;
	char *encoded;
	for (i=0; i<lse->files_cnt; ++i)
	{
			if (strftime(outstr, sizeof(outstr), "%FT%TZ", gmtime(&lse->files_p[i]->mtime)) == 0)
			{
				fprintf(stderr, "strftime returned 0");
				exit(EXIT_FAILURE);
			}
			// http://stackoverflow.com/questions/10323060/printing-file-permissions-like-ls-l-using-stat2-in-c
			fprintf(out, (S_ISDIR(lse->files_p[i]->mode)) ? "d" : "-");
			fprintf(out, (lse->files_p[i]->mode & S_IRUSR) ? "r" : "-");
			fprintf(out, (lse->files_p[i]->mode & S_IWUSR) ? "w" : "-");
			fprintf(out, (lse->files_p[i]->mode & S_IXUSR) ? "x" : "-");
			fprintf(out, (lse->files_p[i]->mode & S_IRGRP) ? "r" : "-");
			fprintf(out, (lse->files_p[i]->mode & S_IWGRP) ? "w" : "-");
			fprintf(out, (lse->files_p[i]->mode & S_IXGRP) ? "x" : "-");
			fprintf(out, (lse->files_p[i]->mode & S_IROTH) ? "r" : "-");
			fprintf(out, (lse->files_p[i]->mode & S_IWOTH) ? "w" : "-");
			fprintf(out, (lse->files_p[i]->mode & S_IXOTH) ? "x" : "-");
			char *bname,*tmp;
			//bname = basename(lse->files_p[i]->realpath);
			tmp = strrchr(lse->files_p[i]->filename,'/');
			if (tmp != NULL)
				bname = tmp + 1;
			else bname = lse->files_p[i]->filename;

			if (encode)
			{
				encoded = pencode(crl,bname);

				fprintf(out,"\t%d\t%d\t%d\t%d\t%s\t%s",
					(int)lse->files_p[i]->nlink,
					lse->files_p[i]->uid,
					lse->files_p[i]->gid,
					(int)lse->files_p[i]->size,
					outstr,
					/*lse->files_p[i]->filename*/
					encoded
				);

				pfree(encoded);
			}
			else
			{
				fprintf(out,"\t%d\t%d\t%d\t%d\t%s\t%s",
					(int)lse->files_p[i]->nlink,
					lse->files_p[i]->uid,
					lse->files_p[i]->gid,
					(int)lse->files_p[i]->size,
					outstr,
					/*lse->files_p[i]->filename*/
					bname
				);
			}

			if (S_ISDIR(lse->files_p[i]->mode))
				fprintf(out,"/");
			fprintf(out,"\n");
	}
	pclean(crl);
	//fprintf(out,"\n");
}
/*
int main(int argc, char *argv[])
{
	struct lsent lse;
	ls(".",&lse);
	display(&lse);
	free_lse(&lse);
	printf("\n");

	return 0;
}
*/
