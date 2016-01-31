#include <sys/stat.h>
FILE* file_open(const char *, int iswrite);
void file_compressto(FILE*, FILE*, size_t*, size_t*);
void file_decompressto(FILE*, FILE*, size_t*, size_t*);
