struct lsent {
	struct filemeta **files_p;
	int files_cnt;
};

void display(struct lsent *,FILE *,int);
void free_lse(struct lsent *);
int ls(char *, struct lsent *);
