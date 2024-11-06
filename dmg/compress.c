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
#endif

static int bz2Compress(unsigned char *inBuffer, size_t inSize,
                       unsigned char *outBuffer, size_t outBufSize, size_t *compSize)
{
  unsigned int bz2CompSize = outBufSize;
  int ret = (BZ2_bzBuffToBuffCompress((char*)outBuffer, &bz2CompSize, (char*)inBuffer, inSize, 9, 0, 0) != BZ_OK);
  *compSize = bz2CompSize;
  return ret;
}

static int zlibCompress(unsigned char *inBuffer, size_t inSize,
                        unsigned char *outBuffer, size_t outBufSize, size_t *compSize)
{
  *compSize = outBufSize;
  return (compress2(outBuffer, compSize, inBuffer, inSize, Z_DEFAULT_COMPRESSION) != Z_OK);
}

int getCompressor(Compressor* comp, char *name)
{
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

  return 1;
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
  } else {
    fprintf(stderr, "Unsupported block type: %#08x\n", type);
    return 1;
  }

  if (ret == 0) {
    ASSERT(decompSize == expectedSize, "Decompressed size mismatch");
  }
  return ret;
}
