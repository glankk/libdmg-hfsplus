#ifndef DMG_COMPRESS_H
#define DMG_COMPRESS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Return zero on success
typedef int (*CompressFunc)(unsigned char *inBuffer, size_t inSize,
                            unsigned char *outBuffer, size_t outBufSize, size_t *compSize,
                            int level);

typedef struct {
  CompressFunc compress;
  int level;
  uint32_t block_type;
} Compressor;

// Pass NULL name to get the default. Return zero on success
int getCompressor(Compressor* comp, char *name);

const char *compressionNames();

// Return zero on success
int decompressRun(uint32_t type,
                  unsigned char* inBuffer, size_t inSize,
                  unsigned char* outBuffer, size_t outBufSize, size_t expectedSize);

#ifdef __cplusplus
}
#endif

#endif