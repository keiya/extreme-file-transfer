#include <limits.h>
struct cwd_ctx {
	char root_path[PATH_MAX+1];
	char cwd_path[PATH_MAX+1];
};

struct cwd_ctx* cwd_init();
int cwd_is_statable(const char*);
void cwd_get_path(struct cwd_ctx*, char *);
int cwd_set_root(struct cwd_ctx*,const char*);
void cwd_realpath(struct cwd_ctx*,const char *,char *);
int cwd_chdir(struct cwd_ctx*,const char*,int);
