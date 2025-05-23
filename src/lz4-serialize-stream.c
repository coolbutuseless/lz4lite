
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lz4.h"


#define MAGIC_LENGTH 8

#define BUF_SIZE 512 * 1024

#define FMODE_READ  0
#define FMODE_WRITE 1

typedef struct {
  FILE *file;
  int fmode;
  uint8_t buf[2][BUF_SIZE];
  int idx;
  uint32_t pos;
  uint32_t data_length;
} dbuf_t;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void db_destroy(dbuf_t *db) {
  if (db->file != NULL) {
    if (db->fmode == FMODE_WRITE) {
      fwrite(&db->pos, 1, sizeof(uint32_t), db->file);
      fwrite(db->buf[db->idx], 1, db->pos, db->file);
    }
    fclose(db->file);
  } else {
    Rf_error("Unknown output in 'db_flush()");
  }
  
  free(db);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read/write single byte callbacks aren't used in binary serialisation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_byte_stream(R_outpstream_t stream, int c) {
  Rf_error("'write_byte_stream()' is never called");
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Write multiple bytes into the buffer at the current location.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void write_bytes_stream(R_outpstream_t stream, void *src, int length) {
  dbuf_t *db = (dbuf_t *)stream->data;
  
  // uint8_t *p = (uint8_t *)src;
  // for (int i = 0; i < length; i++) {
  //   Rprintf("%02x ", p[i]);
  // }
  // Rprintf("\n");
  
  if (db->pos + length > BUF_SIZE) {
    Rprintf("<<<<<<<<<<<<<<< internal\n");
    fwrite(&db->pos, 1, sizeof(uint32_t), db->file);
    fwrite(db->buf[db->idx], 1, db->pos, db->file);
    db->pos = 0;
    db->idx = 1 - db->idx;
  } 
  
  
  // Append data
  memcpy(db->buf[db->idx] + db->pos, src, length);
  db->pos += length;
  
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



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Never called
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int read_byte_stream(R_inpstream_t stream) {
  Rf_error("'read_byte_stream()' is never called");
  return 0;
}




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read multiple bytes from the serialized stream
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void read_bytes_stream(R_inpstream_t stream, void *dst, int length) {
  dbuf_t *db = (dbuf_t *)stream->data;
  
  // Rprintf("\n[00] %i\n", length);
  if (db->pos + length > db->data_length) {
    // copy across available bytes
    // replenish buffer
    int nbytes = db->data_length - db->pos;
    // Rprintf("SAT Nbytes: %i\n", nbytes);
    memcpy(dst, db->buf[db->idx] + db->pos, nbytes);
    length -= nbytes;
    
    db->pos = 0;
    db->idx = 1 - db->idx;
    fread(&db->data_length, 1, sizeof(uint32_t), db->file);
    // Rprintf("data_length: %i\n", db->data_length);
    unsigned long nread = fread(db->buf[db->idx], 1, db->data_length, db->file);
    if (nread != db->data_length) {
      // Rf_error("Read failed: %i/%i", (int)nread, db->data_length);
    }
  }
  
  // copy across bytes
  memcpy(dst, db->buf[db->idx] + db->pos, length);
  db->pos += length;
  
  // Rprintf("Read outtro: %i\n", db->pos);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Unpack a raw vector to an R object
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SEXP lz4_unserialize_stream_(SEXP src_) {

  dbuf_t *db = calloc(1, sizeof(dbuf_t));
  if (db == NULL) {
    Rf_error("Couldn't allocate double buffer");
  }
  
  if (TYPEOF(src_) == STRSXP) {
    const char *filename = CHAR(STRING_ELT(src_, 0));
    db->file = fopen(filename, "rb");
    if (db->file == NULL) {
      Rf_error("Couldn't open file for uboyt: '%s'", filename);
    }
    db->fmode = FMODE_READ;
  } else {
    Rf_error("Don't know how to deal with 'src' of type: [%i] %s", 
             TYPEOF(src_), Rf_type2char(TYPEOF(src_)));
  }


  // Treat the data buffer as an input stream
  struct R_inpstream_st input_stream;

  R_InitInPStream(
    &input_stream,           // Stream object wrapping data buffer
    (R_pstream_data_t) db,  // Actual data buffer
    R_pstream_any_format,    // Unpack all serialized types
    read_byte_stream,        // Function to read single byte from buffer
    read_bytes_stream,       // Function for reading multiple bytes from buffer
    NULL,                    // Func for special handling of reference data.
    NULL                     // Data related to reference data handling
  );

  // Unserialize the input_stream into an R object
  SEXP res_  = PROTECT(R_Unserialize(&input_stream));

  db_destroy(db);
  UNPROTECT(1);
  return res_;
}

