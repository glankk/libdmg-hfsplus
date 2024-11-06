#include "dmg/dmg.h"
#include "dmg/compress.h"

#include <zlib.h>
#include <bzlib.h>
#include "dmg/adc.h"

#ifdef HAVE_LIBLZMA
  #include <lzma.h>

  static int lzmaDecompress(unsigned char* inBuffer, size_t inSize, unsigned char* outBuffer, size_t outBufSize, size_t *decompSize)
  {
    lzma_ret lret;
    uint64_t memlimit = UINT64_MAX;
    size_t inPos = 0;
    *decompSize = 0;

    lret = lzma_stream_buffer_decode(&memlimit, LZMA_FAIL_FAST, NULL,
      inBuffer, &inPos, inSize, outBuffer, decompSize, outBufSize);
    return lret != LZMA_OK;
  }

  static int lzmaCompress(unsigned char *inBuffer, size_t inSize,
                          unsigned char *outBuffer, size_t outBufSize, size_t *compSize, int level)
  {
    lzma_ret lret;

    *compSize = 0;
    if (level == -1)
      level = LZMA_PRESET_DEFAULT;
    lret = lzma_easy_buffer_encode(level, LZMA_CHECK_NONE, NULL, inBuffer, inSize, outBuffer,
      compSize, outBufSize);
    return lret != LZMA_OK;
  }
#endif

#ifdef HAVE_LZFSE
  #include <lzfse.h>

  static int lzfseDecompress(unsigned char* inBuffer, size_t inSize, unsigned char* outBuffer, size_t outBufSize, size_t *decompSize)
  {
    *decompSize = lzfse_decode_buffer(outBuffer, outBufSize, inBuffer, inSize, NULL);
    return !*decompSize;
  }

  static int lzfseCompress(unsigned char *inBuffer, size_t inSize,
                          unsigned char *outBuffer, size_t outBufSize, size_t *compSize, int level)
  {
    *compSize = lzfse_encode_buffer(outBuffer, outBufSize, inBuffer, inSize, NULL);
    return !*compSize;
  }
#endif

static int bz2Compress(unsigned char *inBuffer, size_t inSize,
                       unsigned char *outBuffer, size_t outBufSize, size_t *compSize, int level)
{
  unsigned int bz2CompSize = outBufSize;
  if (level == -1)
    level = 9;
  int ret = (BZ2_bzBuffToBuffCompress((char*)outBuffer, &bz2CompSize, (char*)inBuffer, inSize, level, 0, 0) != BZ_OK);
  *compSize = bz2CompSize;
  return ret;
}

static int zlibCompress(unsigned char *inBuffer, size_t inSize,
                        unsigned char *outBuffer, size_t outBufSize, size_t *compSize, int level)
{
  *compSize = outBufSize;
  if (level == -1)
    level = Z_DEFAULT_COMPRESSION;
  return (compress2(outBuffer, compSize, inBuffer, inSize, level) != Z_OK);
}

int getCompressor(Compressor* comp, char *name)
{
  if (name == NULL) {
    comp->level = -1;
  }

  if (name == NULL || strcasecmp(name, "bzip2") == 0) {
    comp->block_type = BLOCK_BZIP2;
    comp->compress = bz2Compress;
    return 0;
  }
  if (strcasecmp(name, "zlib") == 0) {
    comp->block_type = BLOCK_ZLIB;
    comp->compress = zlibCompress;
    return 0;
  }
#ifdef HAVE_LIBLZMA
  if (strcasecmp(name, "lzma") == 0) {
    comp->block_type = BLOCK_LZMA;
    comp->compress = lzmaCompress;
    return 0;
  }
#endif
#ifdef HAVE_LZFSE
  if (strcasecmp(name, "lzfse") == 0) {
    comp->block_type = BLOCK_LZFSE;
    comp->compress = lzfseCompress;
    return 0;
  }
#endif

  return 1;
}

const char *compressionNames()
{
  return "bzip2, zlib"
#ifdef HAVE_LIBLZMA
    ", lzma"
#endif
#ifdef HAVE_LZFSE
    ", lzfse"
#endif
  ;
}

int decompressRun(uint32_t type,
                  unsigned char* inBuffer, size_t inSize,
                  unsigned char* outBuffer, size_t outBufSize, size_t expectedSize)
{
  size_t decompSize;
  int ret;

  if (type == BLOCK_ADC) {
    ret = (adc_decompress(inSize, inBuffer, outBufSize, outBuffer, &decompSize) != inSize);
  } else if (type == BLOCK_ZLIB) {
    decompSize = outBufSize;
    ret = (uncompress(outBuffer, &decompSize, inBuffer, inSize) != Z_OK);
  } else if (type == BLOCK_BZIP2) {
    unsigned int bz2DecompSize = outBufSize;
    ret = (BZ2_bzBuffToBuffDecompress((char*)outBuffer, &bz2DecompSize, (char*)inBuffer, inSize, 0, 0) != BZ_OK);
    decompSize = bz2DecompSize;
#ifdef HAVE_LIBLZMA
  } else if (type == BLOCK_LZMA) {
    ret = lzmaDecompress(inBuffer, inSize, outBuffer, outBufSize, &decompSize);
#endif
#ifdef HAVE_LZFSE
  } else if (type == BLOCK_LZFSE) {
    ret = lzfseDecompress(inBuffer, inSize, outBuffer, outBufSize, &decompSize);
#endif
  } else {
    fprintf(stderr, "Unsupported block type: %#08x\n", type);
    return 1;
  }

  if (ret == 0) {
    ASSERT(decompSize == expectedSize, "Decompressed size mismatch");
  }
  return ret;
}
