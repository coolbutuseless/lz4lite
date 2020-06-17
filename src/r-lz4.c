
#include <R.h>
#include <Rinternals.h>

#include "lz4.h"
#include "lz4hc.h"

/*  from: https://cran.r-project.org/doc/manuals/r-release/R-ints.html
 SEXPTYPE	Description
  0	  0  NILSXP	      NULL
  1	  1  SYMSXP	      symbols
  2	  2  LISTSXP	    pairlists
  3	  3  CLOSXP	      closures
  4	  4  ENVSXP	      environments
  5	  5  PROMSXP	    promises
  6	  6  LANGSXP	    language objects
  7	  7  SPECIALSXP	  special functions
  8	  8  BUILTINSXP	  builtin functions
  9	  9  CHARSXP	    internal character strings
 10	  a  LGLSXP	      logical vectors
 11   b
 12   c
 13	  d  INTSXP	      integer vectors
 14	  e  REALSXP	    numeric vectors
 15	  f  CPLXSXP	    complex vectors
 16	 10  STRSXP	      character vectors
 17	 11  DOTSXP	      dot-dot-dot object
 18	 12  ANYSXP	      make “any” args work
 19	 13  VECSXP	      list (generic vector)
 20	 14  EXPRSXP	    expression vector
 21	 15  BCODESXP	    byte code
 22	 16  EXTPTRSXP	  external pointer
 23	 17  WEAKREFSXP	  weak reference
 24	 18  RAWSXP       raw vector
 25	 19  S4SXP	      S4 classes not of simple type
 */



/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * compress a raw block of data at the given acceleration
 *
 * @param src_ buffer to be compressed. Raw bytes.
 * @param acc_ acceleration. integer
 * LZ4_compress_fast (const char* src, char* dst, int srcSize, int dstCapacity, int acceleration);
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
SEXP lz4_compress_(SEXP src_, SEXP acceleration_, SEXP compressionLevel_, SEXP HC_) {

  /* SEXP type will determine multiplier for data size */
  int sexp_type = TYPEOF(src_);
  int srcSize   = length(src_);
  int mult;
  char *src;

  /* Get a pointer to the data */
  src = (char *)DATAPTR(src_);

 /* adjust 'srcSize' for the given datatype */
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
    error("compress() cannot handles SEXP type: %i\n", sexp_type);
  }

  /* calculate maximum possible size of compressed buffer in the worst case */
  int dstCapacity  = LZ4_compressBound(srcSize);

  /* Create a temporary buffer to hold anything up to this size */
  char *dst;
  dst = (char *)malloc(dstCapacity);
  if (!dst) {
    error("Couldn't allocate compression buffer of size: %i\n", dstCapacity);
  }

  /* Compression function returns an integer, which is non-zero, positive
   * integer if successful (representing length), or a 0 or negative number
   * in case of an error
   */
  int status;

  if (asLogical(HC_)) {
    status = LZ4_compress_HC(src, dst, srcSize, dstCapacity, asInteger(compressionLevel_));
  } else {
    status = LZ4_compress_fast (src, dst, srcSize, dstCapacity, asInteger(acceleration_));
  }

  /* Watch for badness */
  if (status <= 0) {
    error("Compression error. Status: %i", status);
  }

  /* Allocate a raw R vector of the exact length hold the compressed data
   * and memcpy just the compressed bytes into it.
   * Allocate 8 more bytes than necessary so that there is a minimal header
   * at the front of the compressed data with
   *  - 3 bytes: magic bytes: LZ4
   *  - 1 byte: SEXP type
   *  - 4 bytes: Number of bytes of uncompressed data (32 bit integer)
   */
  SEXP rdst = PROTECT(allocVector(RAWSXP, status + 8));
  char *rdstp = (char *)RAW(rdst);
  int *header = (int *)RAW(rdst);
  memcpy(rdstp + 8, dst, status);

  /* Do some header twiddling */
  header[1] = srcSize;
  rdstp[0] = 'L'; /* 'LZ4' */
  rdstp[1] = 'Z'; /* 'LZ4' */
  rdstp[2] = '4'; /* 'LZ4' */
  rdstp[3] = (char)sexp_type;    /* Store SEXP type here */

  free(dst);
  UNPROTECT(1);
  return rdst;
}



/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Decompress a raw block
 *
 * @param src_ buffer to be decompressed. Raw bytes. The first 8 bytes of this
 *        must be a header with bytes[0:2] = 'LZ4'.  Byte[3] is the SEXPTYPE.
 *        bytes[4:7] represent a 32bit integer with the uncompressed length
 *
 * int LZ4_decompress_safe (const char* src, char* dst, int compressedSize, int dstCapacity);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
SEXP lz4_decompress_(SEXP src_) {

  /* Some pointers into the buffer */
  char *src = (char *)RAW(src_);
  int *isrc = (int *)RAW(src_);

  /* Check the magic bytes are correct i.e. there is a header with length info */
  if (src[0] != 'L' | src[1] != 'Z' | src[2] != '4') {
    error("Buffer must be LZ4 data compressed with 'lz4lite'. 'LZ4' expected as header, but got - '%c%c%c'", src[0], src[1], src[2]);
  }

  /* Find the number of bytes in src and final decompressed size.
   * Need to account for the 8byte header
   */
  int compressedSize = length(src_) - 8;
  int dstCapacity = isrc[1];

  /* Create a decompression buffer of the exact required size and do decompression */
  SEXP dst_;
  char *dst;
  int sexp_type = src[3];

  switch(sexp_type) {
  case LGLSXP:
    dst_ = PROTECT(allocVector(LGLSXP, dstCapacity/4));
    dst = (char *)LOGICAL(dst_);
    break;
  case INTSXP:
    dst_ = PROTECT(allocVector(INTSXP, dstCapacity/4));
    dst = (char *)INTEGER(dst_);
    break;
  case REALSXP:
    dst_ = PROTECT(allocVector(REALSXP, dstCapacity/8));
    dst = (char *)REAL(dst_);
    break;
  case RAWSXP:
    dst_ = PROTECT(allocVector(RAWSXP, dstCapacity));
    dst = (char *)RAW(dst_);
    break;
  case CPLXSXP:
    dst_ = PROTECT(allocVector(CPLXSXP, dstCapacity/16));
    dst = (char *)COMPLEX(dst_);
    break;
  default:
    error("decompress() cannot handles SEXP type: %i\n", sexp_type);
  }


  /* Deompression function returns an integer, which is non-zero, positive
   * integer if successful (representing length), or a 0 or negative number
   * in case of an error
   */
  int status = LZ4_decompress_safe(src + 8, dst, compressedSize, dstCapacity);

  /* Watch for badness */
  if (status <= 0) {
    error("De-compression error. Status: %i", status);
  }

  UNPROTECT(1);
  return dst_;
}
