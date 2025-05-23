
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lz4.h"

#include "buffer-static.h"
#include "calc-size-robust.h"

#define MAGIC_LENGTH 8

#define BUF_SIZE 512 * 1024

#define FMODE_READ  0
#define FMODE_WRITE 1

typedef struct {
  FILE *file;
  int fmode;
  uint8_t buf[2][BUF_SIZE];
  int idx;
  int nbytes;
  int nwritten;
  size_t pos;
} dbuf_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read/write single byte callbacks aren't used in binary serialisation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_byte_stream(R_outpstream_t stream, int c) {
  Rf_error("'write_byte_stream()' is never called");
}

int read_byte_stream(R_inpstream_t stream) {
  Rf_error("'read_byte_stream()' is never called");
  return 0;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write multiple bytes into the buffer at the current location.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_bytes_stream(R_outpstream_t stream, void *src, int length) {
  dbuf_t *db = (dbuf_t *)stream->data;
  
  db->nbytes += length;
  
  if (db->pos + length > BUF_SIZE) {
    // Write buffer
    Rprintf("switch\n");
    db->nwritten += db->pos;
    fwrite(db->buf[db->idx], 1, db->pos, db->file);
    db->pos = 0;
    db->idx = 1 - db->idx;
  } 
  
  
  // Append data
  db->pos += length;
  memcpy(db->buf[db->idx] + db->pos, src, length);
  
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read multiple bytes from the serialized stream
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void read_bytes_stream(R_inpstream_t stream, void *dst, int length) {
  dbuf_t *db = (dbuf_t *)stream->data;
  // memcpy(dst, buf->data + buf->pos, length);
  db->pos += length;
}




void db_destroy(dbuf_t *db) {
  if (db->file != NULL) {
    if (db->fmode == FMODE_WRITE) {
      fwrite(db->buf[db->idx], 1, db->pos, db->file);
    }
    fclose(db->file);
  } else {
    Rf_error("Unknown output in 'db_flush()");
  }
  
  db->nwritten += db->pos;
  Rprintf("Bytes: %i / %i\n", db->nbytes, db->nwritten);
  
  free(db);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Serialize an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP lz4_serialize_stream_(SEXP x_, SEXP dst_) {
  
  dbuf_t *db = calloc(1, sizeof(dbuf_t));
  if (db == NULL) {
    Rf_error("Couldn't allocate double buffer");
  }
  
  if (TYPEOF(dst_) == STRSXP) {
    const char *filename = CHAR(STRING_ELT(dst_, 0));
    db->file = fopen(filename, "wb");
    if (db->file == NULL) {
      Rf_error("Couldn't open file for output: '%s'", filename);
    }
    db->fmode = FMODE_WRITE;
  } else {
    Rf_error("Don't know how to deal with 'dst' of type: [%i] %s", 
             TYPEOF(dst_), Rf_type2char(TYPEOF(dst_)));
  }
  

  // Create the output stream structure
  struct R_outpstream_st output_stream;

  // Initialise the output stream structure
  R_InitOutPStream(
    &output_stream,          // The stream object which wraps everything
    (R_pstream_data_t) db,   // The actual data
    R_pstream_binary_format, // Store as binary
    3,                       // Version = 3 for R >3.5.0 See `?base::serialize`
    write_byte_stream,       // Function to write single byte to buffer
    write_bytes_stream,      // Function for writing multiple bytes to buffer
    NULL,                    // Func for special handling of reference data.
    R_NilValue               // Data related to reference data handling
  );

  // Serialize the object into the output_stream
  R_Serialize(x_, &output_stream);

  db_destroy(db);
  return R_NilValue;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack a raw vector to an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP lz4_unserialize_stream_(SEXP src_) {

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Sanity check
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (TYPEOF(src_) != RAWSXP) {
    Rf_error("unpack(): Only raw vectors can be unserialized");
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // A C pointer into the raw data
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const char *src = (const char *)RAW(src_);
  const int *isrc = (const int *)src;

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Check the magic bytes are correct i.e. there is a header with length info
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (src[0] != 'L' || src[1] != 'Z' || src[2] != '4' || src[3] != 'S') {
    Rf_error("lzr_unserialize(): Buffer must be LZ4 data compressed with 'lz4lite'. 'LZ40' expected as header, but got - '%c%c%c%c'",
          src[0], src[1], src[2], src[3]);
  }


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find the number of bytes of compressed data in the frame
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int compressedSize = Rf_length(src_) - MAGIC_LENGTH;
  int dstCapacity = isrc[1];


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Create a decompression buffer of the exact required size
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  void *dst = malloc((size_t)dstCapacity);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Decompress
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  int status = LZ4_decompress_safe(src + MAGIC_LENGTH, dst, compressedSize, dstCapacity);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Watch for decompression errors
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (status <= 0) {
    Rf_error("lz4_unserialize(): De-compression error. Status: %i", status);
  }


  // Create a buffer object which points to the raw data
  static_buffer_t *buf = malloc(sizeof(static_buffer_t));
  if (buf == NULL) {
    Rf_error("'buf' malloc failed!");
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

