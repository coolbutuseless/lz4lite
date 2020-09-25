



#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lz4.h"
#include "lz4hc.h"

#include "buffer-static.h"
#include "calc-size-robust.h"

#define MAGIC_LENGTH 8

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP lz4_serialize_(SEXP robj, SEXP acceleration_, SEXP use_hc_, SEXP compressionLevel_) {

  // Create the buffer for the serialized representation
  // See also: `expand_buffer()` which re-allocates the memory buffer if
  // it runs out of space
  int start_size = calc_size_robust(robj);

  static_buffer_t *buf = init_buffer(start_size);


  // Create the output stream structure
  struct R_outpstream_st output_stream;

  // Initialise the output stream structure
  R_InitOutPStream(
    &output_stream,          // The stream object which wraps everything
    (R_pstream_data_t) buf,  // The actual data
    R_pstream_binary_format, // Store as binary
    3,                       // Version = 3 for R >3.5.0 See `?base::serialize`
    write_byte,              // Function to write single byte to buffer
    write_bytes,             // Function for writing multiple bytes to buffer
    NULL,                    // Func for special handling of reference data.
    R_NilValue               // Data related to reference data handling
  );

  // Serialize the object into the output_stream
  R_Serialize(robj, &output_stream);

  int srcSize = buf->pos;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // calculate maximum possible size of compressed buffer in the worst case
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int dstCapacity  = LZ4_compressBound(srcSize);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a temporary buffer to hold anything up to this size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  char *dst;
  dst = (char *)malloc(dstCapacity);
  if (!dst) {
    error("Couldn't allocate compression buffer of size: %i\n", dstCapacity);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Compress the data into the temporary buffer
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int num_compressed_bytes;
  if (asLogical(use_hc_)) {
    num_compressed_bytes = LZ4_compress_HC ((const char *)buf->data, dst, srcSize, dstCapacity, asInteger(compressionLevel_));
  } else {
    num_compressed_bytes = LZ4_compress_fast ((const char *)buf->data, dst, srcSize, dstCapacity, asInteger(acceleration_));
  }

  /* Watch for badness */
  if (num_compressed_bytes <= 0) {
    error("Compression error. Status: %i", num_compressed_bytes);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Allocate a raw R vector of the exact length hold the compressed data
  // and memcpy just the compressed bytes into it.
  // Allocate more bytes than necessary so that there is a minimal header
  // at the front of the compressed data with
  //  - 3 bytes: magic bytes: LZ4
  //  - 1 byte: SEXP type
  //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  SEXP rdst = PROTECT(allocVector(RAWSXP, num_compressed_bytes + MAGIC_LENGTH));
  char *rdstp = (char *)RAW(rdst);



  memcpy(rdstp + MAGIC_LENGTH, dst, num_compressed_bytes);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Set 3 magic bytes for the header, and 1 byte for ndims + sexp type
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  rdstp[0] = 'L'; /* LZ4' */
  rdstp[1] = 'Z'; /* LZ4' */
  rdstp[2] = '4'; /* LZ4' */
  rdstp[3] =  0;  /* Store SEXP type here */

  int *header = (int *)rdstp;
  header[1] = srcSize;

  // Free all the memory
  free(buf->data);
  free(buf);
  UNPROTECT(1);
  return rdst;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack a raw vector to an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP lz4_unserialize_(SEXP src_) {

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (TYPEOF(src_) != RAWSXP) {
    error("unpack(): Only raw vectors can be unserialized");
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // A C pointer into the raw data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char *src = (const char *)RAW(src_);
  const int *isrc = (const int *)src;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Check the magic bytes are correct i.e. there is a header with length info
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (src[0] != 'L' || src[1] != 'Z' || src[2] != '4') {
    error("lzr_unserialize(): Buffer must be LZ4 data compressed with 'lz4lite'. 'LZ4' expected as header, but got - '%c%c%c'",
          src[0], src[1], src[2]);
  }

  if (src[3] != 0) {
    error("lz4_unserialize(): Input was not created with 'lz4_serialize()'.  Perhaps you meant to use 'lz4_uncompress()'?");
  }


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find the number of bytes of compressed data in the frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int compressedSize = length(src_) - MAGIC_LENGTH;
  int dstCapacity = isrc[1];


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a decompression buffer of the exact required size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void *dst = malloc(dstCapacity);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Decompress
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int status = LZ4_decompress_safe(src + MAGIC_LENGTH, dst, compressedSize, dstCapacity);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Watch for decompression errors
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (status <= 0) {
    error("lz4_unserialize(): De-compression error. Status: %i", status);
  }


  // Create a buffer object which points to the raw data
  static_buffer_t *buf = malloc(sizeof(static_buffer_t));
  if (buf == NULL) {
    error("'buf' malloc failed!");
  }
  buf->length = dstCapacity;
  buf->pos    = 0;
  buf->data   = dst;

  // Treat the data buffer as an input stream
  struct R_inpstream_st input_stream;

  R_InitInPStream(
    &input_stream,           // Stream object wrapping data buffer
    (R_pstream_data_t) buf,  // Actual data buffer
    R_pstream_any_format,    // Unpack all serialized types
    read_byte,               // Function to read single byte from buffer
    read_bytes,              // Function for reading multiple bytes from buffer
    NULL,                    // Func for special handling of reference data.
    NULL                     // Data related to reference data handling
  );

  // Unserialize the input_stream into an R object
  SEXP res_  = PROTECT(R_Unserialize(&input_stream));

  free(buf);
  UNPROTECT(1);
  return res_;
}

