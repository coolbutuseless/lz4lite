
#include <R.h>
#include <Rinternals.h>

#include "lz4.h"

#define MAGIC_LENGTH 8

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Compress atomic vectors
//
// @param src_ buffer to be compressed. Raw bytes.
// @param acc_ acceleration. integer
// LZ4_compress_fast (const char* src, char* dst, int srcSize, int dstCapacity, int acceleration);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP lz4_compress_(SEXP src_) {

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // SEXP type will determine multiplier for data size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int srcSize   = length(src_);
  void *src = (char *)RAW(src_);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // calculate maximum possible size of compressed buffer in the worst case
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int dstCapacity  = LZ4_compressBound(srcSize);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate 8 more bytes than necessary so that there is a minimal header
  // at the front of the compressed data with
  //  - 3 bytes: magic bytes: LZ4
  //  - 1 byte: SEXP type
  //  - 4 bytes: Number of bytes of uncompressed data (32 bit integer)
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dst_ = PROTECT(allocVector(RAWSXP, dstCapacity + MAGIC_LENGTH));
  char *dst = (char *)RAW(dst_);


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //Compression function returns an integer, which is non-zero, positive
  //integer if successful (representing length), or a 0 or negative number
  //in case of an error
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int num_compressed_bytes;
  num_compressed_bytes = LZ4_compress_default(src, dst + MAGIC_LENGTH, srcSize, dstCapacity);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Watch for compression failure
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (num_compressed_bytes <= 0) {
    error("Compression error. Status: %i", num_compressed_bytes);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Add some magic bytes as the first 4 bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  dst[0] = 'L'; /* 'LZ4' */
  dst[1] = 'Z'; /* 'LZ4' */
  dst[2] = '4'; /* 'LZ4' */
  dst[3] = 'C';

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Put the total uncompressed size as the second 4 bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int *header = (int *)dst;
  header[1] = srcSize;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Adjust actual length of compressed data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  dst_ = PROTECT(Rf_lengthgets(dst_, num_compressed_bytes + MAGIC_LENGTH));

  UNPROTECT(2);
  return dst_;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Decompress a raw block
//
// @param src_ buffer to be decompressed. Raw bytes. The first 8 bytes of this
//        must be a header with bytes[0:2] = 'LZ4'.  Byte[3] is the SEXPTYPE.
//        bytes[4:7] represent a 32bit integer with the uncompressed length
//
// int LZ4_decompress_safe (const char* src, char* dst, int compressedSize, int dstCapacity);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP lz4_decompress_(SEXP src_) {

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Some pointers into the buffer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char *src = (const char *)RAW(src_);
  const int *isrc = (const int *)src;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Check the magic bytes are correct i.e. there is a header with length info
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (src[0] != 'L' || src[1] != 'Z' || src[2] != '4' || src[3] != 'C') {
    error("Buffer must be LZ4 data compressed with 'lz4lite'. 'LZ4C' expected as header, but got - '%c%c%c%c'", src[0], src[1], src[2], src[3]);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find the number of bytes in src and final decompressed size.
  // Need to account for the 8byte header
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int compressedSize = length(src_) - MAGIC_LENGTH;
  int dstCapacity = isrc[1];


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a decompression buffer of the exact required size and do decompression
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP dst_ = PROTECT(allocVector(RAWSXP, dstCapacity));
  void *dst = (void *)RAW(dst_);


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Deompression function returns an integer, which is non-zero, positive
  // integer if successful (representing length), or a 0 or negative number
  // in case of an error
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int status = LZ4_decompress_safe(src + MAGIC_LENGTH, dst, compressedSize, dstCapacity);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Watch for badness
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (status <= 0) {
    error("De-compression error. Status: %i", status);
  }

  UNPROTECT(1);
  return dst_;
}
