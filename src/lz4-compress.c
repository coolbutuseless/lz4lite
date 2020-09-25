
#include <R.h>
#include <Rinternals.h>

#include "lz4.h"
#include "lz4hc.h"

#define MAGIC_LENGTH 8

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Compress atomic vectors
//
// @param src_ buffer to be compressed. Raw bytes.
// @param acc_ acceleration. integer
// LZ4_compress_fast (const char* src, char* dst, int srcSize, int dstCapacity, int acceleration);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP lz4_compress_(SEXP src_, SEXP acceleration_, SEXP use_hc_, SEXP compressionLevel_) {

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // SEXP type will determine multiplier for data size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int sexp_type = TYPEOF(src_);
  int srcSize   = length(src_);
  int mult;
  void *src;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Get a pointer to the data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  src = (void *)DATAPTR(src_);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // adjust 'srcSize' for the given datatype
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  switch(sexp_type) {
  case LGLSXP:
  case INTSXP:
    srcSize *= 4;
    break;
  case REALSXP:
    srcSize *= 8;
    break;
  case CPLXSXP:
    srcSize *= 16;
    break;
  case RAWSXP:
    break;
  default:
    error("lz4_compress() only handles numeric values in atomic vectors , not '%s'.  Try 'lz4_serialize()' instead.", Rf_type2char(sexp_type));
  }



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
  SEXP rdst = PROTECT(allocVector(RAWSXP, dstCapacity + MAGIC_LENGTH));
  char *dst = (char *)RAW(rdst);


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //Compression function returns an integer, which is non-zero, positive
  //integer if successful (representing length), or a 0 or negative number
  //in case of an error
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int num_compressed_bytes;
  if (asLogical(use_hc_)) {
    num_compressed_bytes = LZ4_compress_HC(src, dst + MAGIC_LENGTH, srcSize, dstCapacity, asInteger(compressionLevel_));
  } else {
    num_compressed_bytes = LZ4_compress_fast(src, dst + MAGIC_LENGTH, srcSize, dstCapacity, asInteger(acceleration_));
  }

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
  dst[3] = (char)sexp_type;    /* Store SEXP type here */

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Put the total uncompressed size as the second 4 bytes
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int *header = (int *)dst;
  header[1] = srcSize;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Adjust actual length of compressed data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SETLENGTH(rdst, num_compressed_bytes + MAGIC_LENGTH);

  UNPROTECT(1);
  return rdst;
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
SEXP lz4_uncompress_(SEXP src_) {

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Some pointers into the buffer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char *src = (const char *)RAW(src_);
  const int *isrc = (const int *)src;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Check the magic bytes are correct i.e. there is a header with length info
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (src[0] != 'L' || src[1] != 'Z' || src[2] != '4') {
    error("Buffer must be LZ4 data compressed with 'lz4lite'. 'LZ4' expected as header, but got - '%c%c%c'", src[0], src[1], src[2]);
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
  SEXP dst_;
  void *dst;
  int sexp_type = src[3];

  switch(sexp_type) {
  case LGLSXP:
    dst_ = PROTECT(allocVector(LGLSXP, dstCapacity/4));
    dst = (void *)LOGICAL(dst_);
    break;
  case INTSXP:
    dst_ = PROTECT(allocVector(INTSXP, dstCapacity/4));
    dst = (void *)INTEGER(dst_);
    break;
  case REALSXP:
    dst_ = PROTECT(allocVector(REALSXP, dstCapacity/8));
    dst = (void *)REAL(dst_);
    break;
  case RAWSXP:
    dst_ = PROTECT(allocVector(RAWSXP, dstCapacity));
    dst = (void *)RAW(dst_);
    break;
  case CPLXSXP:
    dst_ = PROTECT(allocVector(CPLXSXP, dstCapacity/16));
    dst = (void *)COMPLEX(dst_);
    break;
  default:
    error("decompress() cannot handles SEXP type: %i\n", sexp_type);
  }

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
