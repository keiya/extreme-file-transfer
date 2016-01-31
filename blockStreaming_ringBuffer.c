// LZ4 streaming API example : ring buffer
// Based on sample code from Takayuki Matsuoka


/**************************************
 * Compiler Options
 **************************************/
#ifdef _MSC_VER    /* Visual Studio */
#  define _CRT_SECURE_NO_WARNINGS // for MSVC
#  define snprintf sprintf_s
#endif
#ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wmissing-braces"   /* GCC bug 53119 : doesn't accept { 0 } as initializer (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53119) */
#endif


/**************************************
 * Includes
 **************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "lz4/lib/lz4.h"


enum {
    MESSAGE_MAX_BYTES   = 1024,
    RING_BUFFER_BYTES   = 1024 * 256 + MESSAGE_MAX_BYTES,
    //DECODE_RING_BUFFER  = RING_BUFFER_BYTES + MESSAGE_MAX_BYTES   // Intentionally larger, to test unsynchronized ring buffers
    DECODE_RING_BUFFER  = RING_BUFFER_BYTES
};


size_t write_int32(FILE* fp, int32_t i) {
    return fwrite(&i, sizeof(i), 1, fp);
}

size_t write_bin(FILE* fp, const void* array, int arrayBytes) {
    return fwrite(array, 1, arrayBytes, fp);
}

size_t read_int32(FILE* fp, int32_t* i) {
    return fread(i, sizeof(*i), 1, fp);
}

size_t read_bin(FILE* fp, void* array, int arrayBytes) {
    return fread(array, 1, arrayBytes, fp);
}


void test_compress(FILE* outFp, FILE* inpFp, size_t* outsz, size_t* insz)
{
    LZ4_stream_t lz4Stream_body = { 0 };
    LZ4_stream_t* lz4Stream = &lz4Stream_body;

    static char inpBuf[RING_BUFFER_BYTES];
    int inpOffset = 0;

    for(;;) {
        // Read random length ([1,MESSAGE_MAX_BYTES]) data to the ring buffer.
        char* const inpPtr = &inpBuf[inpOffset];
        //const int randomLength = (rand() % MESSAGE_MAX_BYTES) + 1;
        //const int inpBytes = (int) read_bin(inpFp, inpPtr, randomLength);
        const int inpBytes = (int) read_bin(inpFp, inpPtr, MESSAGE_MAX_BYTES);
        if (0 == inpBytes) break;
		*insz += inpBytes;

        {
            char cmpBuf[LZ4_COMPRESSBOUND(MESSAGE_MAX_BYTES)];
            const int cmpBytes = LZ4_compress_continue(lz4Stream, inpPtr, cmpBuf, inpBytes);
            if(cmpBytes <= 0) break;
            *outsz += write_int32(outFp, cmpBytes);
            *outsz += write_bin(outFp, cmpBuf, cmpBytes);

            inpOffset += inpBytes;

            // Wraparound the ringbuffer offset
            if(inpOffset >= RING_BUFFER_BYTES - MESSAGE_MAX_BYTES) inpOffset = 0;
        }
    }

    *outsz += write_int32(outFp, 0);
}


void test_decompress(FILE* outFp, FILE* inpFp, size_t* outsz, size_t* insz)
{
    static char decBuf[DECODE_RING_BUFFER];
    int   decOffset    = 0;
    LZ4_streamDecode_t lz4StreamDecode_body = { 0 };
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;

    for(;;) {
        int cmpBytes = 0;
        char cmpBuf[LZ4_COMPRESSBOUND(MESSAGE_MAX_BYTES)];

        {
            const size_t r0 = read_int32(inpFp, &cmpBytes);
            if(r0 != 1 || cmpBytes <= 0) break;
			*insz += r0;

            const size_t r1 = read_bin(inpFp, cmpBuf, cmpBytes);
            if(r1 != (size_t) cmpBytes) break;
			*insz += r1;
        }

        {
            char* const decPtr = &decBuf[decOffset];
            const int decBytes = LZ4_decompress_safe_continue(
                lz4StreamDecode, cmpBuf, decPtr, cmpBytes, MESSAGE_MAX_BYTES);
            if(decBytes <= 0) break;
            decOffset += decBytes;
            *outsz += write_bin(outFp, decPtr, decBytes);

            // Wraparound the ringbuffer offset
            if(decOffset >= DECODE_RING_BUFFER - MESSAGE_MAX_BYTES) decOffset = 0;
        }
    }
}

/*
int main(int argc, char** argv)
{
    if(argc < 2) {
        printf("Please specify mode\n");
        return 0;
    }

    // compress
    if (strcmp(argv[1],"c")==0) {
        test_compress(stdout, stdin);
    }

    // decompress
    if (strcmp(argv[1],"x")==0) {
        test_decompress(stdout, stdin);
    }

    return 0;
}
*/
