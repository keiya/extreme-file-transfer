#include <sys/stat.h>
FILE* file_open(const char *, int iswrite);
void file_compressto(FILE*, FILE*);
void file_decompressto(FILE*, FILE*);
