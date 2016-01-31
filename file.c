#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "blockStreaming_ringBuffer.h"
#include "lz4/lib/lz4.h"
#define	FILEIO_BUFFERSIZE	4096
FILE* file_open(const char *filename, int iswrite)
{
	FILE* fp;
	fp = fopen(filename,
			//writable ? O_CREAT|O_TRUNC|S_IWRITE : O_RDONLY,
			iswrite ? "wb" : "rb");
	if (fp == NULL) {
		perror(filename);
		return NULL;
	}

	/*
	if (fstat(fileno(fp), fs) < 0) {
		perror("fstat");
		exit(-1);
	}
	*/
	return fp;
}

inline void file_compressto(FILE* infp, FILE* outfp, size_t* insz, size_t* outsz)
{
	test_compress(outfp, infp, outsz, insz);
}

inline void file_decompressto(FILE* infp, FILE* outfp, size_t* insz, size_t* outsz)
{
	test_decompress(outfp, infp, outsz, insz);
}
